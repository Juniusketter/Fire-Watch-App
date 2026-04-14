
it('enforces user language regardless of Accept-Language', async () => {
  const res = await request(app)
    .get('/api/me')
    .set('Accept-Language', 'en-US')
    .set('Authorization', tokenEsUser);

  expect(res.body.language).toBe('es');
});
