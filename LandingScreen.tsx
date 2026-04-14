
import { useTranslation } from 'react-i18next';
import { useNavigate } from 'react-router-dom';

export default function LandingScreen() {
  const { t, i18n } = useTranslation();
  const navigate = useNavigate();

  const toggleLang = () => {
    const next = i18n.language === 'es' ? 'en' : 'es';
    navigate(`/lang/${next}`);
  };

  return (
    <div>
      <button onClick={toggleLang}>
        {i18n.language === 'es' ? 'English' : 'Español'}
      </button>

      <h1>{t('dash')}</h1>
      <button>{t('lIn')}</button>
    </div>
  );
}
