
import { useEffect } from 'react';
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router-dom';

export default function LanguageRoute({ lang }) {
  const { i18n } = useTranslation();
  const navigate = useNavigate();

  useEffect(() => {
    i18n.changeLanguage(lang);
    localStorage.setItem('firewatch_language', lang);
    navigate('/', { replace: true });
  }, [lang]);

  return null;
}
