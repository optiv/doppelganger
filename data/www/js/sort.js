var capturedCards;

fetch("cards.csv")
  .then((response) => response.text())
  .then((csvData) => {
    capturedCards = parseCSV(csvData);
    buildTable(capturedCards);
  });

var thElements = document.querySelectorAll("th");
for (var i = 0; i < thElements.length; i++) {
  thElements[i].addEventListener("click", function () {
    var column = this.getAttribute("data-column");
    var order = this.getAttribute("data-order");
    var text = this.innerHTML;
    text = text.substring(0, text.length);

    if (order == "desc") {
      capturedCards = capturedCards.sort((a, b) =>
        a[column] > b[column] ? 1 : -1
      );
      this.setAttribute("data-order", "asc");
      text;
    } else {
      capturedCards = capturedCards.sort((a, b) =>
        a[column] < b[column] ? 1 : -1
      );
      this.setAttribute("data-order", "desc");
      text;
    }

    this.innerHTML = text;
    buildTable(capturedCards);
  });
}

function parseCSV(csvData) {
  var lines = csvData.split("\n");
  var data = [];

  for (var i = 0; i < lines.length; i++) {
    var line = lines[i].trim();
    if (line) {
      var parts = line.split(",");
      var BL = parseInt(parts[0].split(":")[1].trim());
      var FC = parseInt(parts[2].split(":")[1].trim());
      var CN = parseInt(parts[3].split(":")[1].trim());

      if (FC !== 0 || CN !== 0) {
        data.push({ BL, FC, CN });
      }
    }
  }

  return data;
}

function buildTable(data) {
  var table = document.getElementById("cardTable");
  table.innerHTML = "";
  for (var i = 0; i < data.length; i++) {
    var row = `<tr>
                <td>${data[i].BL}</td>
                <td>${data[i].FC}</td>
                <td>${data[i].CN}</td>
           </tr>`;
    table.innerHTML += row;
  }
}
