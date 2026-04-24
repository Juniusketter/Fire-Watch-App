import { SystemStatus } from './status.enum';

export const STATUS_META = {
  ACTIVE: { color: 'green', aria: 'Active' },
  INACTIVE: { color: 'gray', aria: 'Inactive' },
  ALARM: { color: 'red', aria: 'Alarm' },
  FAULT: { color: 'orange', aria: 'Fault' }
} satisfies Record<SystemStatus, unknown>;
