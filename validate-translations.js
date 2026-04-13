const fs = require("fs");

const BASE_LANG = "en";
const TARGET_LANGS = ["es"]; // add more languages as needed

function loadJSON(path) {
  try {
    return JSON.parse(fs.readFileSync(path, "utf8"));
  } catch (err) {
    console.error(`❌ JSON parse error in ${path}`);
    console.error(err.message);
    process.exit(1);
  }
}

function walk(base, target, path = "") {
  let errors = [];

  for (const key of Object.keys(base)) {
    const fullKey = path ? `${path}.${key}` : key;

    if (!(key in target)) {
      errors.push(`❌ Missing key: ${fullKey}`);
      continue;
    }

    if (typeof base[key] === "object" && typeof target[key] === "object") {
      errors.push(...walk(base[key], target[key], fullKey));
    } else {
      if (!target[key] || target[key].trim() === "") {
        errors.push(`❌ Empty translation: ${fullKey}`);
      }

      if (target[key] === base[key]) {
        errors.push(`⚠️ Possibly untranslated (same as EN): ${fullKey}`);
      }
    }
  }

  return errors;
}

const base = loadJSON(`./locales/${BASE_LANG}.json`);
let failed = false;

for (const lang of TARGET_LANGS) {
  const target = loadJSON(`./locales/${lang}.json`);
  const errors = walk(base, target);

  if (errors.length) {
    failed = true;
    console.error(`
🚨 Translation issues in ${lang}.json:
`);
    errors.forEach(e => console.error(e));
  } else {
    console.log(`✅ ${lang}.json passed validation`);
  }
}

if (failed) {
  console.error("
❌ Translation validation failed
");
  process.exit(1);
} else {
  console.log("
✅ All translations valid
");
}
