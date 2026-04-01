document.querySelectorAll(".cutscene-container > img").forEach((a) => a.addEventListener("click", () => a.remove()));
document.getElementById("start-button").addEventListener("click", () => {


  document.getElementById("lobby-handler").remove();
  document.getElementById("main-display").style.display = "grid";

  document.querySelector("body").requestFullscreen();
  screen.orientation.lock("landscape");


let get_inputs = Module.cwrap('get_inputs', 'string', [])
function step(_) {
  const inp = get_inputs();



  const msg = JSON.parse(inp);

  if (msg["type"] == "set_game_state") {
    renderLayout(msg["game_state"]);
    if (msg["game_state"] == 2) {
      let buttons = ["button-a", "button-b", "button-x", "button-y"];
      for(let item in msg["upgrades"]){
        document.querySelector(`#shop-display > div> #${buttons[item]}`).className = `img-btn img-btn-${msg["upgrades"][item]}`;
      }

    }
  }

  requestAnimationFrame(step);
}

requestAnimationFrame(step);
  // socket.addEventListener("message", (e) => {
  //   const msg = JSON.parse(e.data);
  //
  //   if (msg["type"] == "set_game_state") {
  //     renderLayout(msg["game_state"]);
  //     if (msg["game_state"] == 2) {
  //       let buttons = ["button-a", "button-b", "button-x", "button-y"];
  //       for(let item in msg["upgrades"]){
  //         document.querySelector(`#shop-display > div> #${buttons[item]}`).className = `img-btn img-btn-${msg["upgrades"][item]}`;
  //       }
  //
  //     }
  //   }
  // });

  try {
    let gyroscope = new Gyroscope({ frequency: 10 });

    gyroscope.addEventListener("reading", (e) => {
      // socket.send(
      //   JSON.stringify({
      //     type: "gyro_update",
      //     x: gyroscope.x,
      //     y: gyroscope.y,
      //     z: gyroscope.z,
      //   }),
      // );
    });
    gyroscope.start();
  } catch {
    alert("Gyro not found, please use arrow keys.")
  }

  let send_inputs = Module.cwrap('send_inputs', 'void', ['string'])
  let buttons = ["a", "b", "x", "y"];
  for (let button in buttons) {
    document.querySelectorAll(`#button-${buttons[button]}`).forEach((a) =>
      a.addEventListener("mousedown", (e) => {
        send_inputs(JSON.stringify({ type: "button_down", button: button }));
      }),
    );
    document.querySelectorAll(`#button-${buttons[button]}`).forEach((a) =>
      a.addEventListener("mouseup", (e) => {
        send_inputs(JSON.stringify({ type: "button_up", button: button }));
      }),
    );
  }
});

function renderLayout(layout) {
  switch (layout) {
    case 0:
    case 3:
    case 4:
      renderMain();
      break;
    case 1:
      renderGame();
      break;
    case 2:
      renderShop();
      break;
  }
}

function renderShop() {
  document.getElementById("shop-display").style.display = "grid";
  document.getElementById("main-display").style.display = "none";
  document.getElementById("game-display").style.display = "none";
  document.getElementById("win-display").style.display = "none";
}

function renderMain() {
  document.getElementById("shop-display").style.display = "none";
  document.getElementById("main-display").style.display = "grid";
  document.getElementById("game-display").style.display = "none";
  document.getElementById("win-display").style.display = "none";
}

function renderGame() {
  document.getElementById("main-display").style.display = "none";
  document.getElementById("shop-display").style.display = "none";
  document.getElementById("game-display").style.display = "grid";
  document.getElementById("win-display").style.display = "none";
}

function renderWin() {
  document.getElementById("main-display").style.display = "none";
  document.getElementById("shop-display").style.display = "none";
  document.getElementById("game-display").style.display = "none";
  document.getElementById("win-display").style.display = "grid";
}

