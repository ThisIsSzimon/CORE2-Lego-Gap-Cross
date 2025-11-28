function setStatus(text) {
  const el = document.getElementById('status');
  if (el) {
    el.textContent = text;
  }
}

async function sendCmd(c) {
  try {
    const res = await fetch("/cmd?c=" + encodeURIComponent(c));
    if (!res.ok) {
      setStatus("Blad HTTP: " + res.status);
      return;
    }
    const txt = await res.text();
    console.log("CMD", c, "resp:", txt);
  } catch (e) {
    console.error(e);
    setStatus("Blad polaczenia: " + e);
  }
}

function attachHoldButton(btnId, cmdDown, cmdUp, label) {
  const btn = document.getElementById(btnId);
  if (!btn) {
    console.error("Brak przycisku:", btnId);
    return;
  }

  let isActive = false;

  function start(e) {
    e.preventDefault();
    if (isActive) return;
    isActive = true;

    setStatus("START: " + label + " (cmd: " + cmdDown + ")");
    sendCmd(cmdDown);
  }

  function stop(e) {
    e.preventDefault();
    if (!isActive) return;
    isActive = false;

    setStatus("STOP: " + label + " (cmd: " + cmdUp + ")");
    sendCmd(cmdUp);
  }

  btn.addEventListener('mousedown', start);
  btn.addEventListener('mouseup', stop);
  btn.addEventListener('mouseleave', stop);

  btn.addEventListener('touchstart', start);
  btn.addEventListener('touchend', stop);
  btn.addEventListener('touchcancel', stop);

  btn.addEventListener('contextmenu', e => e.preventDefault());
}

window.addEventListener('load', () => {
  attachHoldButton('btnBridgeUp',   'A', 'X', 'Most: Góra');
  attachHoldButton('btnBridgeDown', 'a', 'X', 'Most: Dół');

  attachHoldButton('btnCarBridgeFwd',  'B', 'Y', 'Pojazd na moście: Przód');
  attachHoldButton('btnCarBridgeBack', 'b', 'Y', 'Pojazd na moście: Tył');

  attachHoldButton('btnCarDeskFwd',  'C', 'Z', 'Pojazd na biurku: Przód');
  attachHoldButton('btnCarDeskBack', 'c', 'Z', 'Pojazd na biurku: Tył');
});
