var connection = new WebSocket("ws://" + location.hostname + ":81/");

function eraseCards() {
  if (confirm("Are you sure you want to erase the stored card data?")) {
    alert("Erasing stored card data...");
    let cardData = JSON.stringify({ WIPE_CARDS: true });
    connection.send(cardData);

    setTimeout(function () {
      window.location.reload();
    }, 2000);
  } else {
  }
}

function restoreConfig() {
  if (confirm("Are you sure you want to reset the notification settings?")) {
    alert("Clearing stored notification settings...");
    let configData = JSON.stringify({ WIPE_CONFIG: true });
    connection.send(configData);

    setTimeout(function () {
      window.location.reload();
    }, 2000);
  } else {
  }
}

function resetWireless() {
  if (
    confirm("Are you sure you want to clear the stored wireless credentials?")
  ) {
    alert("Removing stored wireless credentials and rebooting...");
    let configData = JSON.stringify({ WIPE_WIFI: true });
    connection.send(configData);

    window.location.reload();
  } else {
  }
}

function fullReset() {
  if (confirm("Are you sure you want to perform a full factory reset?")) {
    alert(
      "Clearing stored card data, erasing e-mail configuration, wiping wireless credentials and rebooting..."
    );
    let configData = JSON.stringify({ WIPE_DEVICE: true });
    connection.send(configData);

    window.location.reload();
  } else {
  }
}
