
import { Router } from 'express';
const router = Router();

router.get('/me', (req, res) => {
  res.json({
    id: req.user.id,
    role: req.user.role,
    language: req.user.language
  });
});

export default router;
