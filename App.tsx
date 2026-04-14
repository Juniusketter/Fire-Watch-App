
import { BrowserRouter, Routes, Route } from 'react-router-dom';
import LandingScreen from './screens/LandingScreen';
import LanguageRoute from './routes/LanguageRoute';

export default function App() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path='/' element={<LandingScreen />} />
        <Route path='/lang/en' element={<LanguageRoute lang="en" />} />
        <Route path='/lang/es' element={<LanguageRoute lang="es" />} />
      </Routes>
    </BrowserRouter>
  );
}
