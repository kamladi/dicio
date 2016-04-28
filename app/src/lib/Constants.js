export const API_BASE_URL = 'http://localhost:3000';
// export const API_BASE_URL = 'http://128.237.207.54:3000';
// export const API_BASE_URL = 'https://rkmanuwnat.localtunnel.me';
export const WEBSOCKET_URL = API_BASE_URL.replace('https','ws').replace('http','ws');
export const API_OUTLETS_URL = `${API_BASE_URL}/outlets`;
export const API_EVENTS_URL = `${API_BASE_URL}/events`;
export const API_GROUPS_URL = `${API_BASE_URL}/groups`;
export const API_GRAPHS_URL = `${API_BASE_URL}/graphs`;


export const SENSORS = ['light', 'temperature','time'];

export function hasPowerSensor(outlet) {
	return outlet.hardware_version == 'd1c10000';
}

export function hasLightSensor(outlet) {
	return outlet.hardware_version == 'd1c10001';
}
