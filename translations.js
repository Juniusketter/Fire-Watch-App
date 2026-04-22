import express from 'express';
import { db } from '../db.js';

const router = express.Router();

router.get('/:lang', async (req, res) => {
  const rows = await db.all(
    'SELECT translation_key, value FROM translations WHERE language_code=?',
    [req.params.lang]
  );
  res.json(Object.fromEntries(rows.map(r => [r.translation_key, r.value])));
});

export default router;