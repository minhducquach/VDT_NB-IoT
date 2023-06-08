$(document).ready(function () {
  // Tạo bản đồ và đặt tâm ở vị trí ban đầu
  var map = L.map("map").setView([10.834652, 106.766828], 13);

  // Thêm Layer bản đồ từ OpenStreetMap
  L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    maxZoom: 18,
  }).addTo(map);

  // Lấy dữ liệu từ Adafruit IO
  var aioFeedKey = "aio_ytot39or1nRzN5T2KMuSJrO8yCrR"; // Thay YOUR_FEED_KEY bằng khóa Feed của bạn
  var aioUsername = "minhduco19"; // Thay YOUR_USERNAME bằng tên người dùng của bạn
  var aioFeedName = "nb-iot-tracking.nb-iot"; // Thay YOUR_FEED_NAME bằng tên Feed của bạn

  var aioURL =
    "https://io.adafruit.com/api/v2/" +
    aioUsername +
    "/feeds/" +
    aioFeedName +
    "/data";

  function getDataAndUpdateMap() {
    $.ajax({
      url: aioURL,
      method: "GET",
      dataType: "json",
      headers: {
        "X-AIO-Key": aioFeedKey,
      },
      success: function (data) {
        // Xóa các đối tượng đánh dấu hiện tại trên bản đồ
        map.eachLayer(function (layer) {
          if (layer instanceof L.Marker) {
            map.removeLayer(layer);
          }
        });
        var values = {};
        var latestData = data.slice(0, 10);
        // Xử lý dữ liệu mới và hiển thị lên bản đồ
        latestData.forEach(function (item) {
          var lat = item.lat;
          var lon = item.lon;
          var value = item.value;
          //console.log(value);
          var st = value;
          var colorStr;
          // Tách chuỗi thành các cặp key-value
          var pairs = st.split(",");
          pairs.forEach(function (pair) {
            // Tách cặp key-value thành key và value
            var parts = pair.split(":");
            var key = parts[0];
            var val = parts[1];

            // Lưu trữ giá trị vào đối tượng
            values[key] = val;
          });
          console.log(values);
          // Đánh giá chất lượng dựa trên giá trị dữ liệu
          var markerColor = "#ff0000"; // Mặc định màu đỏ
          var temp = parseInt(values["rsrp"]);
          console.log(temp);
          if (temp >= -80 && temp < 0) {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-green.png";
            // markerColor = "#00ff00"; // Màu xanh lá cây cho chất lượng tốt
          } else if (temp >= -90 && temp < -80) {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-gold.png";
            // markerColor = "#ffff00"; // Màu vàng cho chất lượng trung bình
          } else if (temp >= -90 && temp < -100) {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-orange.png";
            // markerColor = "#ffa500"; // Màu cam cho chất lượng không tốt
          } else {
            colorStr =
              "https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-red.png";
            // markerColor = "#ff0000"; // Màu đỏ cho chất lượng xấu
          }
          // Tạo đối tượng đánh dấu và thiết lập màu sắc
          var marker = L.marker([lat, lon], {
            icon: L.icon({
              iconUrl: colorStr,
              iconSize: [25, 41],
              iconAnchor: [12, 41],
              popupAnchor: [1, -34],
              // shadowUrl:
              //   "https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png",
              // shadowSize: [41, 41],
              // shadowAnchor: [12, 41],
              className: "custom-marker",
              html:
                '<div style="background-color:' +
                markerColor +
                '; width: 25px; height: 41px; border-radius: 50%;"></div>',
            }),
          }).addTo(map);
          marker.bindPopup("Giá trị: " + value);
        });
      },
      error: function () {
        console.log("Lỗi khi lấy dữ liệu từ Adafruit IO");
      },
    });
  }

  // Gọi hàm getDataAndUpdateMap ban đầu
  getDataAndUpdateMap();

  // Cập nhật dữ liệu mỗi 5 giây
  setInterval(function () {
    getDataAndUpdateMap();
  }, 5000);
});
