// iPad -> laptop bridge for the study game demo.
// Windows cmd:   set ANTHROPIC_API_KEY=your_key   then   node server.js
// Mac/Linux:     ANTHROPIC_API_KEY=your_key node server.js
// Node 18+, no npm install needed.

const http = require("http");
const fs = require("fs");
const os = require("os");
const path = require("path");

const SERVER_PORT = 3000;
let ANTHROPIC_API_KEY = process.env.ANTHROPIC_API_KEY; // POST /config can replace it
const GENERATION_MODEL = "claude-opus-5-5";
const VISION_MODEL = "claude-sonnet-5-5";
const DEFAULT_ANSWER_TOLERANCE = 0.05; // how close a handwritten decimal must be to count as right
const PROBLEMS_FILE = path.join(__dirname, "problems.json");

// ---------- Problems (loaded from problems.json) ----------
// Each problem lists its parts. A part has a "kind" that decides how it's read and graded:
//   number     one number                  correct: -40
//   point      an (x, y) pair              correct: [-4, -8]
//   numberSet  any number of values,       correct: [-1.3452, 3.3452]   ([] means "none")
//              order doesn't matter
//   choice     one of a few options        correct: "down", options: ["up", "down"]
// Adding a new section = adding problems to problems.json. No code changes.

const PART_KINDS = ["number", "point", "numberSet", "choice"];

function loadProblems() {
  const problemList = JSON.parse(fs.readFileSync(PROBLEMS_FILE, "utf8"));
  for (const problem of problemList) {
    for (const requiredField of ["id", "sectionId", "equation", "parts", "visual"]) {
      if (!(requiredField in problem)) throw new Error(`Problem ${problem.id || "?"} is missing "${requiredField}"`);
    }
    for (const part of problem.parts) {
      if (!PART_KINDS.includes(part.kind)) throw new Error(`Problem ${problem.id}, part ${part.id}: unknown kind "${part.kind}"`);
    }
  }
  return problemList;
}

const PROBLEMS = loadProblems();

// Accepts a problem id ("quadratic-vertex-form") or a 1-based position ("1"), which the iPad buttons send.
function findProblem(problemReference) {
  const byId = PROBLEMS.find((problem) => problem.id === problemReference);
  if (byId) return byId;
  const position = Number(problemReference);
  return Number.isInteger(position) && position >= 1 ? PROBLEMS[position - 1] || null : null;
}

const DEFAULT_UNLOCK_FRACTION = 0.6; // share of the previous section's available XP that opens the next one

let currentProblem = PROBLEMS[0];
let isProblemActive = false; // becomes true once the game starts a problem
let problemStartCount = 0; // goes up on every start, so retrying a problem resets the page too
let hintCount = 0; // hints given on this attempt; resets when the game starts the problem

const GAME_TIMEOUT_MS = 5000; // the game polls /result.json?source=game; this long without it means it closed
let lastGamePollMs = 0; // 0 until the game has polled, so a problem started by hand stays active

let elapsedSecondsByProblem = {}; // time spent on each problem, so coming back resumes the timer
let pausedAtMs = 0; // 0 while the timer runs

function getElapsedSeconds() {
  return ((pausedAtMs || Date.now()) - problemStartTimeMs) / 1000;
}

function rememberElapsedSeconds() {
  if (isProblemActive && currentProblem) elapsedSecondsByProblem[currentProblem.id] = getElapsedSeconds();
}

function endProblem() {
  rememberElapsedSeconds();
  isProblemActive = false;
  lastGamePollMs = 0;
  latestResult = { status: "none" };
}

function hasGameGoneQuiet() {
  return lastGamePollMs > 0 && Date.now() - lastGamePollMs > GAME_TIMEOUT_MS;
}
let problemStartTimeMs = Date.now();
let latestFramePng = null;
let latestFrameProblemId = "";
let latestResult = { status: "none" };

// ---------- Saved board work (shared by every open page) ----------
// problemId -> { revision, work: { documentItems, camera } }. The server is the one source of
// truth, so the iPad, the browser and the game's embedded page all show the same board.
const WORK_FILE = path.join(__dirname, "savedwork.json");
const savedWorkByProblem = loadSavedWorkFile();

function loadSavedWorkFile() {
  try {
    return JSON.parse(fs.readFileSync(WORK_FILE, "utf8"));
  } catch (error) {
    return {};
  }
}

function persistSavedWorkFile() {
  try {
    fs.writeFileSync(WORK_FILE, JSON.stringify(savedWorkByProblem));
  } catch (error) {
    console.log("Could not save board work:", error.message);
  }
}

function storeWork(problemId, work) {
  const previousRevision = savedWorkByProblem[problemId] ? savedWorkByProblem[problemId].revision : 0;
  savedWorkByProblem[problemId] = { revision: previousRevision + 1, work };
  persistSavedWorkFile();
  return savedWorkByProblem[problemId].revision;
}

function readRequestBody(request) {
  return new Promise((resolve) => {
    const bodyChunks = [];
    request.on("data", (chunk) => bodyChunks.push(chunk));
    request.on("end", () => resolve(Buffer.concat(bodyChunks)));
  });
}

function sendJson(response, statusCode, payload) {
  response.writeHead(statusCode, { "Content-Type": "application/json" });
  response.end(JSON.stringify(payload));
}

// ---------- Asking the AI to read the work ----------

// The schema the model must fill in is built from the problem's parts.
function answerSchemaForPart(part) {
  if (part.kind === "number") return { type: ["number", "null"] };
  if (part.kind === "point") {
    return {
      type: ["object", "null"],
      properties: { x: { type: "number" }, y: { type: "number" } },
      required: ["x", "y"],
    };
  }
  if (part.kind === "numberSet") {
    return {
      type: "object",
      properties: {
        values: { type: "array", items: { type: "number" }, description: "Decimal values the student wrote" },
        statedNone: { type: "boolean", description: "Student explicitly said there are none / no real solutions" },
      },
      required: ["values", "statedNone"],
    };
  }
  return { type: "string", enum: [...part.options, "missing"] };
}

function buildReviewTool(problem) {
  const answerProperties = {};
  for (const part of problem.parts) {
    answerProperties[part.id] = { ...answerSchemaForPart(part), description: part.label };
  }
  return {
    name: "submit_review",
    description: "Report what the student wrote for each part, plus feedback.",
    input_schema: {
      type: "object",
      properties: {
        answers: { type: "object", properties: answerProperties, required: problem.parts.map((part) => part.id) },
        firstIncorrectStep: { type: ["string", "null"], description: "One short sentence, or null" },
        feedback: { type: "string" },
        weakSubtopics: { type: "array", items: { type: "string" }, maxItems: 3 },
      },
      required: ["answers", "firstIncorrectStep", "feedback", "weakSubtopics"],
    },
  };
}

function buildReviewPrompt(problem) {
  const answerKeyLines = problem.parts
    .map((part) => `- ${part.label}: ${part.correctText || formatAnswer(part, normalizeCorrectAnswer(part))}`)
    .join("\n");
  return (
    `A student handwrote their work for this problem: "${problem.equation}".\n` +
    `Task: ${problem.instructions || "Solve the problem."}\n` +
    `Correct answers:\n${answerKeyLines}\n\n` +
    `Read the handwriting in the image and call the submit_review tool exactly once. ` +
    `Only report what the student actually wrote; use null (or "missing") for anything missing or unreadable. ` +
    `The student must solve every part themselves. NEVER state, quote or hint at the correct value of a part ` +
    `the student got wrong or left blank, in the feedback or in firstIncorrectStep. For wrong parts, only say ` +
    `which parts need another look and the kind of mistake (for example a sign error), without the right value. ` +
    `Keep feedback to at most 2 short sentences, plain text, no markdown.`
  );
}

async function callAnthropic(requestBody) {
  const apiResponse = await fetch("https://api.anthropic.com/v1/messages", {
    method: "POST",
    headers: {
      "x-api-key": ANTHROPIC_API_KEY,
      "anthropic-version": "2023-06-01",
      "content-type": "application/json",
    },
    body: JSON.stringify(requestBody),
  });
  const apiData = await apiResponse.json();
  if (!apiResponse.ok) {
    throw new Error("API error: " + ((apiData.error && apiData.error.message) || apiResponse.status));
  }
  return apiData;
}

function buildImageBlock(pngBuffer) {
  return { type: "image", source: { type: "base64", media_type: "image/png", data: pngBuffer.toString("base64") } };
}

async function reviewWork(problem, pngBuffer) {
  if (!ANTHROPIC_API_KEY) return buildPlaceholderReview(problem);

  const apiData = await callAnthropic({
    model: VISION_MODEL,
    max_tokens: 2000,
    tools: [buildReviewTool(problem)],
    messages: [
      {
        role: "user",
        content: [buildImageBlock(pngBuffer), { type: "text", text: buildReviewPrompt(problem) }],
      },
    ],
  });
  return gradeReview(extractRawReview(apiData), problem);
}

// ---------- Hints ----------

const MAX_HINT_LEVEL = 3;
const HINT_STRENGTHS = [
  "a gentle nudge about which idea or formula to use",
  "a pointer to the specific step the student should do next",
  "the setup of the next step, stopping before any final value",
];

function buildHintPrompt(problem, hintLevel) {
  const answerKeyLines = problem.parts
    .map((part) => `- ${part.label}: ${part.correctText || formatAnswer(part, normalizeCorrectAnswer(part))}`)
    .join("\n");
  return (
    `A student is working on this problem: "${problem.statement || problem.equation}".\n` +
    `Task: ${problem.instructions || "Solve the problem."}\n` +
    `Answer key (for you only, NEVER reveal any of it):\n${answerKeyLines}\n\n` +
    `Look at the student's work in the image (it may be empty). ` +
    `Give hint ${hintLevel} of ${MAX_HINT_LEVEL}: ${HINT_STRENGTHS[hintLevel - 1]}. ` +
    `Never state a final answer or any answer-key value. ` +
    `At most 2 short sentences, plain text, no markdown. Reply with only the hint.`
  );
}

async function requestHint(problem, pngBuffer, hintLevel) {
  if (!ANTHROPIC_API_KEY) return "Set ANTHROPIC_API_KEY to enable hints.";
  const content = [];
  if (pngBuffer) content.push(buildImageBlock(pngBuffer));
  content.push({ type: "text", text: buildHintPrompt(problem, hintLevel) });
  const apiData = await callAnthropic({
    model: VISION_MODEL,
    max_tokens: 400,
    messages: [{ role: "user", content }],
  });
  const textBlock = (apiData.content || []).find((block) => block.type === "text");
  const hintText = textBlock ? textBlock.text.replace(/[*#`]/g, "").trim() : "";
  if (!hintText) throw new Error("Model returned no hint.");
  return hintText;
}

// The reply can contain several blocks (thinking, text, tool_use); search them all.
function extractRawReview(apiData) {
  const contentBlocks = apiData.content || [];
  const toolBlock = contentBlocks.find((block) => block.type === "tool_use" && block.name === "submit_review");
  if (toolBlock) return toolBlock.input;
  // Fallback: model answered in text instead of calling the tool.
  const textBlock = contentBlocks.find((block) => block.type === "text");
  const replyText = textBlock ? textBlock.text : "";
  const jsonStart = replyText.indexOf("{");
  const jsonEnd = replyText.lastIndexOf("}");
  if (jsonStart === -1 || jsonEnd === -1) throw new Error("Model returned no review.");
  return JSON.parse(replyText.slice(jsonStart, jsonEnd + 1));
}

// ---------- Normalizing and grading (generic for every part kind) ----------

function toNumberOrNull(value) {
  const numberValue = typeof value === "string" ? parseFloat(value) : value;
  return Number.isFinite(numberValue) ? numberValue : null;
}

// Turns whatever the model sent into one clean shape per kind (null = missing).
function normalizeStudentAnswer(part, rawAnswer) {
  if (part.kind === "number") return toNumberOrNull(rawAnswer);
  if (part.kind === "point") {
    if (!rawAnswer) return null;
    const x = toNumberOrNull(Array.isArray(rawAnswer) ? rawAnswer[0] : rawAnswer.x);
    const y = toNumberOrNull(Array.isArray(rawAnswer) ? rawAnswer[1] : rawAnswer.y);
    return x === null || y === null ? null : [x, y];
  }
  if (part.kind === "numberSet") {
    const rawValues = rawAnswer && Array.isArray(rawAnswer.values) ? rawAnswer.values : [];
    const values = rawValues.map(toNumberOrNull).filter((value) => value !== null).sort((first, second) => first - second);
    const statedNone = Boolean(rawAnswer && rawAnswer.statedNone);
    return values.length === 0 && !statedNone ? null : { values, statedNone };
  }
  return part.options.includes(rawAnswer) ? rawAnswer : null;
}

function normalizeCorrectAnswer(part) {
  if (part.kind === "numberSet") {
    const values = [...part.correct].sort((first, second) => first - second);
    return { values, statedNone: values.length === 0 };
  }
  return part.correct;
}

function isClose(studentValue, correctValue, tolerance) {
  return studentValue !== null && Math.abs(studentValue - correctValue) <= tolerance;
}

function isPartCorrect(part, studentAnswer) {
  if (studentAnswer === null) return false;
  const tolerance = part.tolerance ?? DEFAULT_ANSWER_TOLERANCE;
  if (part.kind === "number") return isClose(studentAnswer, part.correct, tolerance);
  if (part.kind === "point") {
    return isClose(studentAnswer[0], part.correct[0], tolerance) && isClose(studentAnswer[1], part.correct[1], tolerance);
  }
  if (part.kind === "numberSet") {
    const correctValues = normalizeCorrectAnswer(part).values;
    if (correctValues.length === 0) return studentAnswer.statedNone && studentAnswer.values.length === 0;
    return studentAnswer.values.length === correctValues.length &&
      correctValues.every((correctValue, index) => isClose(studentAnswer.values[index], correctValue, tolerance));
  }
  return studentAnswer === part.correct;
}

function roundForDisplay(value) {
  return String(Math.round(value * 100) / 100);
}

function formatAnswer(part, answer) {
  if (answer === null || answer === undefined) return null;
  if (part.kind === "number") return roundForDisplay(answer);
  if (part.kind === "point") return `(${roundForDisplay(answer[0])}, ${roundForDisplay(answer[1])})`;
  if (part.kind === "numberSet") return answer.values.length === 0 ? "none" : answer.values.map(roundForDisplay).join(", ");
  return String(answer);
}

// The right answer is only included for parts solved correctly, so a wrong part never gives it away.
function buildPartResult(part, studentAnswer) {
  const isCorrect = isPartCorrect(part, studentAnswer);
  return {
    id: part.id,
    label: part.label,
    isCorrect,
    studentAnswer: formatAnswer(part, studentAnswer),          // null = student didn't write it
    correctAnswer: isCorrect ? part.correctText || formatAnswer(part, normalizeCorrectAnswer(part)) : null,
  };
}

// For drawing: the true shape, plus the shape built from the student's answers (if they gave enough).
function buildVisual(problem, studentAnswersById) {
  const studentParams = {};
  for (const [paramName, source] of Object.entries(problem.visual.studentParams || {})) {
    const answer = studentAnswersById[source.part];
    const value = Array.isArray(answer) ? answer[source.index ?? 0] : answer;
    if (typeof value !== "number") return { type: problem.visual.type, params: problem.visual.params, studentParams: null };
    studentParams[paramName] = value;
  }
  return { type: problem.visual.type, params: problem.visual.params, studentParams };
}

function gradeReview(rawReview, problem) {
  const rawAnswers = (rawReview && rawReview.answers) || {};
  const studentAnswersById = {};
  for (const part of problem.parts) {
    studentAnswersById[part.id] = normalizeStudentAnswer(part, rawAnswers[part.id]);
  }
  const parts = problem.parts.map((part) => buildPartResult(part, studentAnswersById[part.id]));
  return {
    isCorrect: parts.every((partResult) => partResult.isCorrect),
    parts,
    firstIncorrectStep: typeof rawReview.firstIncorrectStep === "string" ? rawReview.firstIncorrectStep : null,
    feedback: String(rawReview.feedback || "").replace(/[*#`]/g, "").trim(),
    weakSubtopics: (Array.isArray(rawReview.weakSubtopics) ? rawReview.weakSubtopics : []).slice(0, 3).map(String),
    visual: buildVisual(problem, studentAnswersById),
  };
}

// Same shape as a real review, so the game can be tested without an API key.
function buildPlaceholderReview(problem) {
  return {
    ...gradeReview({ answers: {}, feedback: "Set ANTHROPIC_API_KEY to enable AI review.", weakSubtopics: [] }, problem),
    isPlaceholder: true,
  };
}

function describeCurrentProblem() {
  return { problemId: currentProblem.id, sectionId: currentProblem.sectionId };
}

// What the iPad page needs to draw the active problem: its text and instructions.
function describeProblemForPage(problem) {
  return {
    problemId: problem.id,
    sectionId: problem.sectionId,
    statement: problem.statement || problem.equation,
    instructions: problem.instructions || "",
  };
}

// Sections in the order they first appear; each one unlocks from the section before it.
function buildCurriculum() {
  const sections = [];
  for (const problem of PROBLEMS) {
    if (sections.some((section) => section.sectionId === problem.sectionId)) continue;
    const previousSection = sections[sections.length - 1];
    sections.push({
      sectionId: problem.sectionId,
      sectionName: problem.sectionName || problem.sectionId,
      prerequisiteSectionId: previousSection ? previousSection.sectionId : null,
      unlockFraction: previousSection ? DEFAULT_UNLOCK_FRACTION : 0,
    });
  }
  return {
    status: "ready",
    subjectName: PROBLEMS[0].subjectName || "General",
    sections,
    problems: PROBLEMS.map((problem) => ({
      id: problem.id,
      sectionId: problem.sectionId,
      statement: problem.statement || problem.equation,
      instructions: problem.instructions || "",
      difficultyRating: describeProblemTopics(problem).difficultyRating,
    })),
  };
}

// What the game needs to file experience under a subject and a section, scaled by difficulty.
function describeProblemTopics(problem) {
  return {
    subjectName: problem.subjectName || "General",
    sectionName: problem.sectionName || problem.sectionId,
    difficultyRating: Number.isFinite(problem.difficultyRating) ? problem.difficultyRating : 1,
  };
}

// ---------- Building problems from uploaded course material ----------
// The upload page sends images or PDFs. The AI turns them into problems in the problems.json format.
let pendingMaterials = null; // upload kept until the game presses Create New World
let materialsStatus = { status: "idle", progress: 0, message: "Waiting for upload..." };

const GENERATED_PROBLEMS_SCHEMA = {
  type: "object",
  properties: {
    subjectName: { type: "string" },
    problems: {
      type: "array",
      items: {
        type: "object",
        properties: {
          id: { type: "string", description: "short kebab-case id, unique" },
          sectionId: { type: "string", description: "kebab-case id of the section" },
          sectionName: { type: "string", description: "readable section name, e.g. 'Section 1.2: Functions'" },
          difficultyRating: { type: "number", description: "1 (easy) to 5 (hard)" },
          equation: { type: "string", description: "plain-text statement, e.g. y = -2(x + 4)^2 - 8" },
          statement: { type: "string", description: "nicely formatted statement with unicode math" },
          instructions: { type: "string" },
          parts: {
            type: "array",
            items: {
              type: "object",
              properties: {
                id: { type: "string" },
                label: { type: "string" },
                kind: { type: "string", enum: ["number", "point", "numberSet", "choice"] },
                options: { type: "array", items: { type: "string" } },
                correct: { description: "number, [x, y], array of numbers, or the chosen option string" },
                correctText: { type: "string" },
              },
              required: ["id", "label", "kind", "correct"],
            },
          },
        },
        required: ["id", "sectionId", "sectionName", "difficultyRating", "equation", "statement", "instructions", "parts"],
      },
    },
  },
  required: ["subjectName", "problems"],
};

function buildMaterialBlock(file) {
  if (file.mediaType === "application/pdf") {
    return { type: "document", source: { type: "base64", media_type: "application/pdf", data: file.data } };
  }
  return { type: "image", source: { type: "base64", media_type: file.mediaType, data: file.data } };
}

async function generateProblemsFromMaterials(files, notes) {
  materialsStatus = { status: "generating", progress: 0.3, message: "Reading your homework..." };
  const apiData = await callAnthropic({
    model: GENERATION_MODEL,
    max_tokens: 16000,
    tools: [{ name: "submit_problems", description: "Submit the problems of the unit.", input_schema: GENERATED_PROBLEMS_SCHEMA }],
    tool_choice: { type: "auto" },
    messages: [{
      role: "user",
      content: [
        ...files.map(buildMaterialBlock),
        { type: "text", text:
          "These are a student's homework or course material for one unit. Turn every assigned problem into a " +
          "gradable problem, keeping the homework's numbering and order exactly. Group problems by the section they " +
          "belong to (one sectionId per section of the unit, in order). Each problem gets 1-4 parts; each part's " +
          "answer must be a single number, an (x, y) point, a set of numbers, or a choice from options. Work out " +
          "the correct answers carefully and double-check them. Give each problem a difficultyRating from 1 to 5. " +
          "When you are done, call the submit_problems tool exactly once with every problem. Do not answer in plain text." +
          (notes ? "\nNotes from the student: " + notes : "") },
      ],
    }],
  });
  materialsStatus = { status: "generating", progress: 0.85, message: "Placing problems in the castles..." };
  const toolUse = apiData.content.find((block) => block.type === "tool_use");
  if (!toolUse || !Array.isArray(toolUse.input.problems) || toolUse.input.problems.length === 0) {
    throw new Error("The AI did not return any problems.");
  }
  return toolUse.input.problems.map((problem) => ({
    ...problem,
    subjectName: toolUse.input.subjectName || "General",
    parts: (problem.parts || []).filter((part) => PART_KINDS.includes(part.kind)),
    visual: { type: "none" },
  }));
}

function replaceProblems(newProblems) {
  fs.writeFileSync(PROBLEMS_FILE, JSON.stringify(newProblems, null, 2));
  PROBLEMS.length = 0;
  PROBLEMS.push(...newProblems);
  currentProblem = PROBLEMS[0];
  isProblemActive = false;
}

async function runMaterialsGeneration(files, notes) {
  try {
    if (!ANTHROPIC_API_KEY) throw new Error("No API key. Enter it in the game's Settings.");
    const newProblems = await generateProblemsFromMaterials(files, notes);
    replaceProblems(newProblems);
    materialsStatus = { status: "ready", progress: 1, message: "Built " + newProblems.length + " problems." };
    console.log("Generated " + newProblems.length + " problems from uploaded material.");
  } catch (error) {
    console.log("Generation failed: " + error);
    materialsStatus = { status: "failed", progress: 0, message: String(error.message || error) };
  }
}

function findLanUrl() {
  for (const interfaceList of Object.values(os.networkInterfaces())) {
    for (const networkInterface of interfaceList) {
      if (networkInterface.family === "IPv4" && !networkInterface.internal) {
        return "http://" + networkInterface.address + ":" + SERVER_PORT;
      }
    }
  }
  return "http://127.0.0.1:" + SERVER_PORT;
}

const server = http.createServer(async (request, response) => {
  const requestUrl = new URL(request.url, "http://localhost");
  const route = request.method + " " + requestUrl.pathname;

  try {
    if (route === "GET /") {
      const pagePath = path.join(__dirname, "index.html");
      if (!fs.existsSync(pagePath)) {
        console.log("ERROR: index.html not found at " + pagePath);
        response.writeHead(500, { "Content-Type": "text/plain" });
        response.end("index.html not found next to server.js. Expected it at:\n" + pagePath);
        return;
      }
      const pageHtml = fs.readFileSync(pagePath);
      response.writeHead(200, { "Content-Type": "text/html" });
      response.end(pageHtml);
    } else if (route === "POST /start") {
      // Selects a problem (by id or 1-based position) and starts its timer.
      const requestedProblem = findProblem(requestUrl.searchParams.get("problem") || "1");
      if (!requestedProblem) {
        sendJson(response, 404, { error: "No such problem" });
        return;
      }
      rememberElapsedSeconds();
      currentProblem = requestedProblem;
      isProblemActive = true;
      pausedAtMs = 0;
      lastGamePollMs = 0;
      problemStartCount += 1;
      hintCount = 0;
      latestFramePng = null;
      latestFrameProblemId = "";
      problemStartTimeMs = Date.now() - (elapsedSecondsByProblem[currentProblem.id] || 0) * 1000;
      latestResult = { status: "in_progress", ...describeCurrentProblem() };
      sendJson(response, 200, {
        ...describeCurrentProblem(),
        equation: currentProblem.equation,
        ...describeProblemForPage(currentProblem),
      });
    } else if (route === "GET /current.json") {
      // The iPad page polls this to learn which problem the game started.
      if (isProblemActive && hasGameGoneQuiet()) endProblem();
      if (!isProblemActive) {
        sendJson(response, 200, { status: "none" });
        return;
      }
      sendJson(response, 200, {
        status: "active",
        startNumber: problemStartCount,
        ...describeProblemForPage(currentProblem),
        elapsedSeconds: getElapsedSeconds(),
        isPaused: pausedAtMs > 0,
      });
    } else if (route === "POST /pause") {
      if (isProblemActive && !pausedAtMs) pausedAtMs = Date.now();
      sendJson(response, 200, { isPaused: pausedAtMs > 0 });
    } else if (route === "POST /resume") {
      if (pausedAtMs) {
        problemStartTimeMs += Date.now() - pausedAtMs;
        pausedAtMs = 0;
      }
      sendJson(response, 200, { isPaused: false });
    } else if (route === "GET /curriculum.json") {
      sendJson(response, 200, buildCurriculum());
    } else if (route === "GET /work") {
      const stored = savedWorkByProblem[requestUrl.searchParams.get("problem")];
      sendJson(response, 200, stored || { revision: 0, work: null });
    } else if (route === "PUT /work") {
      const workProblemId = requestUrl.searchParams.get("problem");
      let work = null;
      try {
        work = JSON.parse((await readRequestBody(request)).toString("utf8"));
      } catch (error) { /* rejected below */ }
      if (!workProblemId || !work || !Array.isArray(work.documentItems)) {
        sendJson(response, 400, { error: "Expected ?problem= and { documentItems, camera }." });
        return;
      }
      sendJson(response, 200, { revision: storeWork(workProblemId, work) });
    } else if (route === "POST /frame") {
      // Live mirror frame (every second or so).
      const framePng = await readRequestBody(request);
      const frameProblemId = requestUrl.searchParams.get("problem");
      if (frameProblemId && (!currentProblem || frameProblemId !== currentProblem.id)) {
        response.writeHead(204); // a late frame from a problem that is no longer active
        response.end();
        return;
      }
      latestFramePng = framePng;
      latestFrameProblemId = currentProblem ? currentProblem.id : "";
      response.writeHead(204);
      response.end();
    } else if (route === "POST /submit") {
      const finalPng = await readRequestBody(request);
      if (!isProblemActive) {
        sendJson(response, 409, { error: "No active problem. Start one in the game first." });
        return;
      }
      latestFramePng = finalPng;
      latestFrameProblemId = currentProblem.id;
      const reviewedProblem = currentProblem;
      const reviewedStartCount = problemStartCount;
      const elapsedSeconds = getElapsedSeconds();
      latestResult = { status: "reviewing", ...describeCurrentProblem(), elapsedSeconds };
      sendJson(response, 202, latestResult); // return fast; review runs below
      try {
        const review = await reviewWork(reviewedProblem, finalPng);
        latestResult = {
          status: review.isPlaceholder ? "stub" : "done",
          problemId: reviewedProblem.id,
          sectionId: reviewedProblem.sectionId,
          ...describeProblemTopics(reviewedProblem),
          elapsedSeconds,
          ...review,
        };
        // A fully solved problem is finished: the page goes back to waiting and can't be submitted again.
        if (review.isCorrect && problemStartCount === reviewedStartCount) {
          elapsedSecondsByProblem[reviewedProblem.id] = elapsedSeconds;
          isProblemActive = false;
        }
      } catch (error) {
        latestResult = {
          status: "error",
          problemId: reviewedProblem.id,
          sectionId: reviewedProblem.sectionId,
          elapsedSeconds,
          message: String(error),
        };
      }
    } else if (route === "POST /hint") {
      // Each hint on the same attempt is stronger than the last, up to a limit.
      if (!isProblemActive) {
        sendJson(response, 409, { error: "No active problem. Start one in the game first." });
        return;
      }
      hintCount = Math.min(hintCount + 1, MAX_HINT_LEVEL);
      const hintLevel = hintCount;
      try {
        const hint = await requestHint(currentProblem, latestFramePng, hintLevel);
        sendJson(response, 200, { hint, hintLevel });
      } catch (error) {
        sendJson(response, 500, { error: String(error) });
      }
    } else if (route === "GET /latest.png") {
      if (!latestFramePng) {
        response.writeHead(404);
        response.end();
      } else {
        response.writeHead(200, {
          "Content-Type": "image/png",
          "Cache-Control": "no-store",
          "X-Problem-Id": latestFrameProblemId,
        });
        response.end(latestFramePng);
      }
    } else if (route === "POST /end") {
      // The game is closing, so the active problem ends with it.
      endProblem();
      response.writeHead(204);
      response.end();
    } else if (route === "POST /config") {
      const config = JSON.parse((await readRequestBody(request)).toString("utf8") || "{}");
      if (typeof config.apiKey === "string" && config.apiKey.trim()) ANTHROPIC_API_KEY = config.apiKey.trim();
      sendJson(response, 200, { hasApiKey: Boolean(ANTHROPIC_API_KEY) });
    } else if (route === "GET /upload") {
      response.writeHead(200, { "Content-Type": "text/html" });
      response.end(fs.readFileSync(path.join(__dirname, "upload.html")));
    } else if (route === "POST /materials") {
      const upload = JSON.parse((await readRequestBody(request)).toString("utf8"));
      if (!Array.isArray(upload.files) || upload.files.length === 0) {
        sendJson(response, 400, { error: "No files uploaded." });
        return;
      }
      pendingMaterials = { files: upload.files, notes: upload.notes || "" };
      materialsStatus = { status: "uploaded", progress: 0.1, message: "Uploaded " + upload.files.length + " file(s). Ready to create." };
      sendJson(response, 202, materialsStatus);
    } else if (route === "POST /generate") {
      if (!pendingMaterials) {
        materialsStatus = { status: "ready", progress: 1, message: "Using the existing problems." };
      } else {
        runMaterialsGeneration(pendingMaterials.files, pendingMaterials.notes);
        pendingMaterials = null;
      }
      sendJson(response, 202, materialsStatus);
    } else if (route === "GET /materials/status") {
      sendJson(response, 200, materialsStatus);
    } else if (route === "GET /lan-url.json") {
      sendJson(response, 200, { uploadUrl: findLanUrl() + "/upload" });
    } else if (route === "GET /result.json") {
      if (requestUrl.searchParams.get("source") === "game") lastGamePollMs = Date.now();
      sendJson(response, 200, latestResult);
    } else {
      response.writeHead(404);
      response.end();
    }
  } catch (error) {
    console.log("Request error on " + route + ": " + error);
    if (!response.headersSent) sendJson(response, 500, { error: String(error) });
    else response.end();
  }
});

server.listen(SERVER_PORT, "0.0.0.0", () => {
  const pageFound = fs.existsSync(path.join(__dirname, "index.html"));
  console.log(pageFound ? "index.html found." : "WARNING: index.html NOT found in " + __dirname);
  console.log("Loaded " + PROBLEMS.length + " problems from problems.json: " + PROBLEMS.map((problem) => problem.id).join(", "));
  console.log("Server running. Open one of these on the iPad (same Wi-Fi):");
  for (const interfaceList of Object.values(os.networkInterfaces())) {
    for (const networkInterface of interfaceList) {
      if (networkInterface.family === "IPv4" && !networkInterface.internal) {
        console.log("  http://" + networkInterface.address + ":" + SERVER_PORT);
      }
    }
  }
});
