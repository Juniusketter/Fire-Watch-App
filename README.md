# Firewatch UI Language Toggle (Qt / C++)

## Files
- TranslationStore.h/.cpp: JSON-based translation loader
- Translator.h/.cpp: Applies translations recursively to the UI

## How to Use

1. Add trKey to widgets:
```
ui->loginButton->setProperty("trKey", "lIn");
```

2. Toggle language:
```
TranslationStore::loadLanguage("es");
applyTranslations(this);
```
