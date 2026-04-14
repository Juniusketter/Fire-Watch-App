
import { AppProvider } from '../context/AppContext';
import Routes from './Routes';

export default function AppRoot({ user }) {
  return (
    <AppProvider user={user}>
      <Routes />
    </AppProvider>
  );
}
