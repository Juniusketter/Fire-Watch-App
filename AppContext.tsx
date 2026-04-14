
import React, { createContext, useContext, useEffect, useState } from 'react';
import { useTranslation } from 'react-i18next';

const AppContext = createContext(null);

export function AppProvider({ children, user }) {
  const { i18n } = useTranslation();
  const [language, setLanguage] = useState(localStorage.getItem('firewatch_language') || 'en');

  useEffect(() => {
    i18n.changeLanguage(language);
  }, [language]);

  return (
    <AppContext.Provider value={{
      role: user?.role,
      language,
      setLanguage
    }}>
      {children}
    </AppContext.Provider>
  );
}

export const useAppContext = () => useContext(AppContext);
