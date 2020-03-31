
function setCollapsibles() {
    var coll = document.getElementsByClassName("collapsible");
    var i;

    for (i = 0; i < coll.length; i++) {
        coll[i].addEventListener("click", function () {
            this.classList.toggle("active");
            var content = this.nextElementSibling;
            if (content.style.display === "block") {
                content.style.display = "none";
            } else {
                content.style.display = "block";
            }
        } );
    }
}
function refreshData() {
    status = fetch('./metrics', { mode: 'no-cors', cache: 'no-cache', redirect: 'follow', referrerPolicy: 'no-referrer' })
    .then(
        (response) => {
            if (!response.ok) { throw Error(response.statusText); }
            return response.json();
        })
    .then(
        (s) => {
            document.getElementById("metricList").innerHTML = "";
            for (var k in s) {
                document.getElementById("metricList").innerHTML = document.getElementById("metricList").innerHTML 
                +"<tr><td>"+k+ "</td><td>"+ s[k]+"</td></tr>";
            }
            console.log("status data updated");
            return s;
        }
    )
    .catch(
        function (err) {
            console.log('Something went wrong.');
            console.log(err);
        }
    );

    status = fetch('./status', { mode: 'no-cors', cache: 'no-cache', redirect: 'follow', referrerPolicy: 'no-referrer' })
        .then(
            (response) => {
                if (!response.ok) { throw Error(response.statusText); }
                return response.json();
            })
        .then(
            (s) => {


                document.getElementById("espChipId").innerHTML = s.espChipId;
                document.getElementById("espFreeSketchSpace").innerHTML = s.espFreeSketchSpace;
                document.getElementById("espSketchSize").innerHTML = s.espSketchSize;
                document.getElementById("espCpuFreqMHz").innerHTML = s.espCpuFreqMHz;
                document.getElementById("espSdkVersion").innerHTML = s.espSdkVersion;
                document.getElementById("espCoreVersion").innerHTML = s.espCoreVersion;
                document.getElementById("espMaxFreeBlockSize").innerHTML = s.espMaxFreeBlockSize;
                document.getElementById("espHeapFragmentation").innerHTML = s.espHeapFragmentation;
                document.getElementById("espFreeHeap").innerHTML = s.espFreeHeap;
                document.getElementById("espResetReason").innerHTML = s.espResetReason;
                document.getElementById("readConfigStatus").innerHTML = s.readConfigStatus;
                document.getElementById("sdsStartLast").innerHTML = s.sdsStartLast;
                document.getElementById("sdsStartTotal").innerHTML = s.sdsStartTotal;
                document.getElementById("sdsLastQueryPM").innerHTML = s.sdsLastQueryPM;
                document.getElementById("sdsLastStatus").innerHTML = s.sdsLastStatus;
                document.getElementById("sdsEndLast").innerHTML = s.sdsEndLast;
                document.getElementById("wifiSSID").innerHTML = s.wifiSSID;
                document.getElementById("wifiStatus").innerHTML = s.wifiStatus;
                document.getElementById("mqttConnServer").innerHTML = s.mqttConnServer;
                document.getElementById("mqttConnPort").innerHTML = s.mqttConnPort;
                document.getElementById("mqttPubPrefix").innerHTML = s.mqttPubPrefix;
                document.getElementById("mqttPubSleep").innerHTML = s.mqttPubSleep;
                document.getElementById("mqttSubPrefix").innerHTML = s.mqttSubPrefix;
                document.getElementById("mqttSubTotal").innerHTML = s.mqttSubTotal;
                document.getElementById("mqttPubTotal").innerHTML = s.mqttPubTotal;
                document.getElementById("mqttPubLastStart").innerHTML = s.mqttPubLastStart;
                document.getElementById("mqttPubLastEnd").innerHTML = s.mqttPubLastEnd;
                document.getElementById("sensorCollectSleep").innerHTML = s.sensorCollectSleep;
                console.log("status data updated");
                return s;
            }
        )
        .catch(
            function (err) {
                console.log('Something went wrong.');
                console.log(err);
            }
        );

}
