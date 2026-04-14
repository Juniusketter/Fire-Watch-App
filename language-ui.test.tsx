
it('renders Spanish when user language is es', () => {
  mockUser({ language: 'es' });
  render(<LandingScreen />);

  expect(screen.getByText('Panel de control')).toBeInTheDocument();
});
