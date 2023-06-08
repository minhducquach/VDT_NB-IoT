$(document).ready(function () {
  var map = L.map("map").setView([0.0, 0.0], 13);
  var markerLayer = L.layerGroup().addTo(map); // Create a layer group for markers

  // Add OpenStreetMap tile layer
  L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    maxZoom: 18,
  }).addTo(map);

  var aioFeedKey = "aio_ytot39or1nRzN5T2KMuSJrO8yCrR";
  var aioUsername = "minhduco19";
  var aioFeedName = "nb-iot-tracking.nb-iot";
  var aioURL = `https://io.adafruit.com/api/v2/${aioUsername}/feeds/${aioFeedName}/data`;

  var latestPoint;
  var start = 0;

  function getDataAndUpdateMap() {
    $.ajax({
      url: aioURL,
      method: "GET",
      dataType: "json",
      headers: {
        "X-AIO-Key": aioFeedKey,
      },
      success: function (data) {
        // Clear existing markers on the map
        markerLayer.clearLayers();

        var latestData = data.slice(0, 20);
        latestPoint = latestData[0]; // Get the latest data point
        map.setView([latestPoint.lat, latestPoint.lon]);

        if (start == 0) {
          // Update the map view to the latest point
          map.setView([latestPoint.lat, latestPoint.lon]);
          start = 1;
        }

        latestData.forEach(function (item, index) {
          var lat = item.lat;
          var lon = item.lon;
          var value = item.value;

          var pairs = value.split(",");
          var values = {};
          pairs.forEach(function (pair) {
            var parts = pair.split(":");
            var key = parts[0];
            var val = parts[1];
            values[key] = val;
          });

          var temp = parseInt(values["rsrp"]);
          var colorStr;

          if (temp >= -80 && temp < 0) {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-green.png";
          } else if (temp >= -90 && temp < -80) {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-gold.png";
          } else if (temp >= -90 && temp < -100) {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-orange.png";
          } else {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-red.png";
          }

          var marker = L.marker([lat, lon], {
            icon: L.icon({
              iconUrl: colorStr,
              iconSize: [25, 41],
              iconAnchor: [12, 41],
              popupAnchor: [1, -34],
            }),
          });

          var popupContent = "<table>";
          pairs.forEach(function (pair) {
            var parts = pair.split(":");
            var key = parts[0];
            var val = parts[1];
            if (key == "rsrq" || key == "rsrp") {
              popupContent +=
                "<tr><td><strong>" + key + "</strong></td><td>" + val + " dBm";
              ("</td></tr>");
            } else {
              popupContent +=
                "<tr><td><strong>" +
                key +
                "</strong></td><td>" +
                val +
                "</td></tr>";
            }
          });
          popupContent +=
            "<tr><td><strong>Quality</strong></td><td>" +
            getSignalQuality(temp) +
            "</td></tr>";
          popupContent += "</table>";

          marker.bindPopup(popupContent);
          markerLayer.addLayer(marker); // Add the marker to the layer group
        });

        // Generate and update the data table
        generateTableRows(latestData);
      },
      error: function () {
        console.log("Lỗi khi lấy dữ liệu từ Adafruit IO");
      },
    });
  }

  function getSignalQuality(rsrp) {
    if (rsrp >= -80 && rsrp < 0) {
      return "Good";
    } else if (rsrp >= -90 && rsrp < -80) {
      return "Normal";
    } else if (rsrp >= -100 && rsrp < -90) {
      return "Bad";
    } else {
      return "Very Bad";
    }
  }

  // Toggle the display of the data list container
  function toggleList() {
    $(".list-container").slideToggle();
  }

  // Function to generate table rows from the latest data
  function generateTableRows(data) {
    var table = $("#data-table");
    var tableBody = table.find("tbody");
    tableBody.empty(); // Clear existing table body content

    // Create table rows
    data.forEach(function (item, index) {
      var id = index + 1; // Generate ID from index
      var lat = item.lat;
      var lon = item.lon;
      var value = item.value;
      var createdAt = new Date(item.created_at);

      var pairs = value.split(",");
      var values = {};
      pairs.forEach(function (pair) {
        var parts = pair.split(":");
        var key = parts[0];
        var val = parts[1];
        values[key] = val;
      });

      var temp = parseInt(values["rsrp"]);
      var colorStr;

      if (temp >= -80 && temp < 0) {
        colorStr = "green";
      } else if (temp >= -90 && temp < -80) {
        colorStr = "gold";
      } else if (temp >= -90 && temp < -100) {
        colorStr = "orange";
      } else {
        colorStr = "red";
      }

      var row = $("<tr></tr>")
        .click(function () {
          zoomToMarker(index);
        })
        .appendTo(tableBody);

      $("<td></td>").css("color", colorStr).text(id).appendTo(row);
      $("<td></td>").css("color", colorStr).text(lat).appendTo(row);
      $("<td></td>").css("color", colorStr).text(lon).appendTo(row);
      $("<td></td>").css("color", colorStr).text(values["pci"]).appendTo(row);
      $("<td></td>")
        .css("color", colorStr)
        .text(`${values["rsrp"]} dBm`)
        .appendTo(row);
      $("<td></td>")
        .css("color", colorStr)
        .text(`${values["rsrq"]} dBm`)
        .appendTo(row);
      $("<td></td>").css("color", colorStr).text(values["sinr"]).appendTo(row);
      $("<td></td>")
        .css("color", colorStr)
        .text(values["cellid"])
        .appendTo(row);
      $("<td></td>")
        .css("color", colorStr)
        .text(
          createdAt.toLocaleString("en-GB", {
            day: "2-digit",
            month: "2-digit",
            year: "numeric",
            hour: "numeric",
            minute: "numeric",
            timeZone: "Asia/Bangkok",
          })
        )
        .appendTo(row); // Convert createdAt to desired format
    });
  }

  // Function to zoom and center the map to the marker associated with the clicked row
  function zoomToMarker(index) {
    var marker = markerLayer.getLayers()[index];
    if (marker) {
      map.setView(marker.getLatLng());
      marker.openPopup();
    }
  }

  // Get data and update map initially
  getDataAndUpdateMap();

  // Update data and map every 5 seconds
  setInterval(function () {
    getDataAndUpdateMap();
  }, 300000);

  // Add title to the table
  $("<caption></caption>")
    .addClass("table-title")
    .text("10 LATEST ENTRIES")
    .prependTo("#data-table");

  $(".table-title").css({
    "font-size": "24px",
    "font-weight": "bold",
    padding: "5.5px",
  });

  // Bind the toggleList function to the stripes button click event
  $(".stripes-button").click(function () {
    toggleList();
  });

  // Create a custom control for the color bar legend
  var legendControl = L.control({
    position: "bottomleft",
  });

  legendControl.onAdd = function (map) {
    var legendDiv = L.DomUtil.create("div", "legend");

    legendDiv.innerHTML = `
    <div class="color-bar">
      <span class="color-bar-item" style="background-color: green;"></span>
      <span class="color-bar-item" style="background-color: gold;"></span>
      <span class="color-bar-item" style="background-color: orange;"></span>
      <span class="color-bar-item" style="background-color: red;"></span>
    </div>
    <div class="legend-labels">
      <span class="legend-label">Good</span>
      <span class="legend-label">Normal</span>
      <span class="legend-label">Bad</span>
      <span class="legend-label">Very Bad</span>
    </div>
  `;

    return legendDiv;
  };

  // Add the legend control to the map
  legendControl.addTo(map);
});
