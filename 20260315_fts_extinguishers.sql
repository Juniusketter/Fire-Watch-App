
-- 20260315_fts_extinguishers.sql
-- Enable FTS5-backed search over extinguishers; safe to run multiple times.

-- Create FTS table
CREATE VIRTUAL TABLE IF NOT EXISTS EXTINGUISHER_FTS
USING fts5(
  extinguisher_id UNINDEXED,
  tenant_id       UNINDEXED,
  serial,
  building,
  floor,
  room,
  location_note,
  type,
  status,
  content='',
  tokenize = 'porter'
);

-- Triggers to keep FTS in sync with base table
DROP TRIGGER IF EXISTS trg_ext_inserts;
CREATE TRIGGER trg_ext_inserts AFTER INSERT ON EXTINGUISHER BEGIN
  INSERT INTO EXTINGUISHER_FTS(extinguisher_id, tenant_id, serial, building, floor, room, location_note, type, status)
  VALUES (NEW.extinguisher_id, NEW.company_id, NEW.serial_number, (SELECT name_or_number FROM BUILDING WHERE building_id=NEW.building_id),
          NEW.floor, NEW.room, NEW.location_note, NEW.type, NEW.status);
END;

DROP TRIGGER IF EXISTS trg_ext_updates;
CREATE TRIGGER trg_ext_updates AFTER UPDATE ON EXTINGUISHER BEGIN
  DELETE FROM EXTINGUISHER_FTS WHERE extinguisher_id=OLD.extinguisher_id;
  INSERT INTO EXTINGUISHER_FTS(extinguisher_id, tenant_id, serial, building, floor, room, location_note, type, status)
  VALUES (NEW.extinguisher_id, NEW.company_id, NEW.serial_number, (SELECT name_or_number FROM BUILDING WHERE building_id=NEW.building_id),
          NEW.floor, NEW.room, NEW.location_note, NEW.type, NEW.status);
END;

DROP TRIGGER IF EXISTS trg_ext_deletes;
CREATE TRIGGER trg_ext_deletes AFTER DELETE ON EXTINGUISHER BEGIN
  DELETE FROM EXTINGUISHER_FTS WHERE extinguisher_id=OLD.extinguisher_id;
END;

-- Optional one-time backfill: uncomment to build index from existing rows
-- INSERT INTO EXTINGUISHER_FTS(extinguisher_id, tenant_id, serial, building, floor, room, location_note, type, status)
-- SELECT e.extinguisher_id, e.company_id, e.serial_number, b.name_or_number, e.floor, e.room, e.location_note, e.type, e.status
-- FROM EXTINGUISHER e LEFT JOIN BUILDING b ON b.building_id=e.building_id
-- WHERE NOT EXISTS (SELECT 1 FROM EXTINGUISHER_FTS f WHERE f.extinguisher_id=e.extinguisher_id);
