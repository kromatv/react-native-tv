import RFB from './core/rfb.js';

const KEYSYMS = {
  up: 0xff52,
  down: 0xff54,
  left: 0xff51,
  right: 0xff53,
  enter: 0xff0d,
  back: 0x1008ff26,
  exit: 0x100810ae,
  info: 0x10081166,
  play: 0x1008ff14,
  pause: 0x1008ff31,
  stop: 0x1008ff15,
  rewind: 0x1008ff3e,
  fastForward: 0x1008ff97,
  previous: 0x1008ff16,
  next: 0x1008ff17,
  red: 0x1008ffa3,
  green: 0x1008ffa4,
  yellow: 0x1008ffa5,
  blue: 0x1008ffa6,
};

const KEYBOARD = {
  ArrowUp: 'up',
  ArrowDown: 'down',
  ArrowLeft: 'left',
  ArrowRight: 'right',
  Enter: 'enter',
  Escape: 'back',
  Backspace: 'back',
  ' ': 'play',
};

const RECONNECT_MS = 2000;
const screen = document.getElementById('screen');
const status = document.getElementById('status');
let rfb = null;

function setStatus(text, state) {
  status.textContent = text;
  status.dataset.state = state;
}

function connect() {
  const previous = rfb;
  rfb = null;
  previous?.disconnect();
  const url = `${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/websockify`;
  const session = new RFB(screen, url);
  session.scaleViewport = true;
  session.focusOnClick = false;
  session.background = '#000';
  session.addEventListener('connect', () => setStatus('Connected to the simulator', 'connected'));
  session.addEventListener('disconnect', () => {
    if (rfb !== session) return;
    setStatus('Disconnected. Retrying...', 'disconnected');
    setTimeout(connect, RECONNECT_MS);
  });
  rfb = session;
}

function press(name) {
  const keysym = KEYSYMS[name];
  if (keysym === undefined || rfb === null) return;
  rfb.sendKey(keysym, null);
  const button = document.querySelector(`[data-key="${name}"]`);
  button?.classList.add('pressed');
  setTimeout(() => button?.classList.remove('pressed'), 120);
}

function saveScreenshot() {
  const canvas = screen.querySelector('canvas');
  if (canvas === null) return;
  const link = document.createElement('a');
  link.download = `tizen-simulator-${new Date().toISOString().replaceAll(':', '-')}.png`;
  link.href = canvas.toDataURL('image/png');
  link.click();
}

for (const button of document.querySelectorAll('[data-key]')) {
  button.addEventListener('click', () => press(button.dataset.key));
}
document.getElementById('save').addEventListener('click', saveScreenshot);
document.getElementById('reconnect').addEventListener('click', connect);
window.addEventListener('keydown', (event) => {
  const name = KEYBOARD[event.key];
  if (name === undefined || event.metaKey || event.ctrlKey || event.altKey) return;
  event.preventDefault();
  press(name);
});

connect();
