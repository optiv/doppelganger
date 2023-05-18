var capturedCards;

fetch("cards.json")
  .then((response) => response.json())
  .then((json) => {
    capturedCards = json;
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
