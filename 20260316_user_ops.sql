-- Migration: add FK restrictions and audit support
ALTER TABLE ASSIGNMENT ADD CONSTRAINT fk_assign_user FOREIGN KEY(inspector_user_id) REFERENCES USER(user_id) ON DELETE RESTRICT;