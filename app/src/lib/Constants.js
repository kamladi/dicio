export const API_BASE_URL = 'http://localhost:3000';
// export const API_BASE_URL = 'https://rkmanuwnat.localtunnel.me';
export const WEBSOCKET_URL = API_BASE_URL.replace('https','ws').replace('http','ws');
export const API_OUTLETS_URL = `${API_BASE_URL}/outlets`;
export const API_EVENTS_URL = `${API_BASE_URL}/events`;


export const SENSORS = ['light', 'temperature','time'];
