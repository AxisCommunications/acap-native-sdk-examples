function fetchApi() {
    document.getElementById("count").classList.add("visible");

    fetch("/local/web_server_rev_proxy/my_web_server/api/example")
        .then(function (res) { return res.json(); })
        .then(function (data) {
            document.getElementById("message").textContent = data.message;
            document.getElementById("count").textContent = "Request count: " + data.requestCount;
        })
        .catch(function (err) {
            document.getElementById("message").textContent = "Error: " + err;
        });
}

document.addEventListener("DOMContentLoaded", function () {
    document.getElementById("fetchBtn").addEventListener("click", fetchApi);
});
