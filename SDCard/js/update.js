function sub(obj) {
  var fileName = obj.value.split("\\\\");
  document.getElementById("file-input").innerHTML =
    "   " + fileName[fileName.length - 1];
}

document.querySelector("form").addEventListener("submit", function (e) {
  e.preventDefault();
  var form = document.querySelector("#upload_form");
  var data = new FormData(form);
  var xhr = new XMLHttpRequest();

  xhr.addEventListener("load", function (evt) {
    console.log("success!");
  });
  xhr.addEventListener("error", function (a, b, c) {
    // handle error
  });

  xhr.upload.addEventListener("progress", function (evt) {
    if (evt.lengthComputable) {
      var per = evt.loaded / evt.total;
      document.querySelector("#prg").innerHTML =
        "progress: " + Math.round(per * 100) + "%";
      document.querySelector("#bar").style.width = Math.round(per * 100) + "%";
    }
  });

  xhr.open("POST", "/update");
  xhr.send(data);
});
