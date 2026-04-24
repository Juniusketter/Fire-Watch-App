import { createContext, useEffect, useState } from 'react';

export const LanguageContext = createContext();

export function LanguageProvider({ children }) {
  const [language, setLanguage] = useState('en');
  const [dict, setDict] = useState({});

  useEffect(() => {
    fetch(`/api/translations/${language}`)
      .then(r => r.json())
      .then(setDict);
  }, [language]);

  const t = key => dict[key] || key;

  return (
    <LanguageContext.Provider value={{ language, setLanguage, t }}>
      {children}
    </LanguageContext.Provider>
  );
}