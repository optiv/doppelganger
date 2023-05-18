function main() {
  const enableEmail = document.getElementById("enable_email").value;
  const smtpHost = document.getElementById("smtp_host").value;
  const smtpPort = document.getElementById("smtp_port").value;
  const smtpUser = document.getElementById("smtp_user").value;
  const smtpPass = document.getElementById("smtp_pass").value;
  const smtpRecipient = document.getElementById("smtp_recipient").value;

  let payload = {
    enable_email: enableEmail,
    smtp_host: smtpHost,
    smtp_port: smtpPort,
    smtp_user: smtpUser,
    smtp_pass: smtpPass,
    smtp_recipient: smtpRecipient,
  };

  if (enableEmail === "false") {
    payload = {
      enable_email: "false",
      smtp_host: "smtp.<domain>.com",
      smtp_port: "465",
      smtp_user: "<sender_email>@<domain>.com",
      smtp_pass: "AppPassword",
      smtp_recipient: "<phonenumber>@<carrierdomain>.com",
    };
  }

  if (enableEmail === "true") {
    alert("Notifications will be sent to: " + smtpRecipient);
  } else {
    alert("Notifications have been disabled.");
  }

  const connection = new WebSocket("ws://" + location.hostname + ":81/");

  connection.onerror = function (error) {
    console.error("WebSocket Error ", error);
  };

  connection.onopen = function () {
    console.log("WebSocket connection established.");
    connection.send(JSON.stringify(payload));
  };

  connection.onclose = function () {
    console.log("WebSocket connection closed.");
  };
}
