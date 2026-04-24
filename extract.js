import gettextParser from 'gettext-parser';
import fs from 'fs';

const keys = [
  'app.title',
  'alert.fire.title',
  'alert.fire.body',
  'notification.evac.title'
];

const pot = {
  charset: 'UTF-8',
  translations: {
    '': Object.fromEntries(
      keys.map(k => [k, { msgid: k, msgstr: [''] }])
    )
  }
};

fs.writeFileSync('i18n/messages.pot', gettextParser.po.compile(pot));