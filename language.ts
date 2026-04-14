
export function applyUserLanguage(req, res, next) {
  if (req.user?.language) {
    res.locals.language = req.user.language;
  }
  next();
}
