
import { useEffect } from 'react';
import { useTranslation } from 'react-i18next';
import { useAppContext } from '../context/AppContext';

export function useRoleLanguageSync() {
  const { i18n } = useTranslation();
  const { language } = useAppContext();

  useEffect(() => {
    if (i18n.language !== language) {
      i18n.changeLanguage(language);
    }
  }, [language]);
}
