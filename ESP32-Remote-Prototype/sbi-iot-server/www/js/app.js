
const API_GETSYSINFO = '/api/getsysinfo';
const API_GETLIVEDATA = '/api/getlivedata';

const API_GETSETTINGSJSON_URI = "/api/getsettingsjson";
const API_POSTSETTINGSJSON_URI = "/api/setsettingsjson";

const API_GETSERVERPARAMETERFILEJSON_URI = "/api/getserverparameterfile";
const API_POSTSERVERPARAMETERFILEJSON_URI = "/api/setserverparameterfile";

let mData = 
{
    sysinfos: [],
    livedata: { state: { is_pairing: false}, wireless: { rx: 0,tx: 0, channel: 0}, stove: { is_connected: false, param_cnt: 0 } },
    // Config JSON
    configJSON: ""
};

var app = new Vue({
	el: '#app',
	data() {
        return mData;
    },
    created: function() {
        // Call when page loading is done ...
        window.addEventListener("load", () => this.page_loaded());
    },
	methods: {
        postAction(url, data) {
            fetch(url, {
                method: 'POST',
                headers: {
                  'Content-Type': 'application/json'
                },
                body: data,
                cache: 'default'
              })
              .catch((ex) => 
              {
                  console.error('url: ', url, " error: ", ex);
              });
        },
        automaticUpdate() {
            fetch(API_GETLIVEDATA)
                .then((response) => response.json())
                .then(
                    (data) => 
                    {
                        this.livedata = data;
                        console.log("livedata: ", data);
                        setTimeout(this.automaticUpdate, 500);
                    })
                .catch((ex) => 
                {
                    console.error('getSysInfo', ex);
                    this.sysinfos = null;

                    setTimeout(this.automaticUpdate, 5000);
                });
        },
        page_loaded()
        {
            console.log("page_loaded");
            // Get system informations
            fetch(API_GETSYSINFO)
                .then((response) => response.json())
                .then((data) => this.sysinfos = data.infos)
                .catch((ex) => 
                {
                    console.error('getSysInfo', ex);
                    this.sysinfos = null;
                });

            setTimeout(this.automaticUpdate, 500);
        },
        idStartPairing_Click()
        {
            let url = "";
            if (!this.livedata.state.is_pairing) {
                url = 'action/espnow_startpairing';
            }
            else {
                url = 'action/espnow_stoppairing';
            }
            console.log("idStartPairing_Click");
            this.postAction(url, { test: "coucou" });
        },
        idLoadConfig_OnClick(event)
        {
            console.log("idLoadConfig_OnClick");
            this.configJSON = "";
            // Get system informations
            let isOK = false;
            fetch(API_GETSERVERPARAMETERFILEJSON_URI)
                .then((response) => 
                {
                    isOK = response.ok;
                    return response.text();
                })
                .then((response) => 
                {
                    if (!isOK)
                        throw Error(response);
                    console.log("url: ", API_GETSERVERPARAMETERFILEJSON_URI, " data: ", response);
                    this.configJSON = response;
                })
                .catch((error) => 
                {
                    console.error("url: ", API_GETSERVERPARAMETERFILEJSON_URI, " error: ", error);
                    this.configJSON = error;
                    alert("Error: " + error);
                });
        },
        idSaveConfig_OnClick(event)
        {
            let isOK = false;
            fetch(API_POSTSERVERPARAMETERFILEJSON_URI, {
                method: 'POST', // or 'PUT'
                headers: {
                  'Content-Type': 'application/json',
                },
                body: this.configJSON,
              })
            .then((response) => 
            {
                isOK = response.ok;
                return response.text();
            })
            .then((response) => {
                if (!isOK)
                    throw Error(response);
                console.log("url: ", API_POSTSERVERPARAMETERFILEJSON_URI, " data: ", response);
            })
            .catch((error) => {
                console.error("url: ", API_POSTSERVERPARAMETERFILEJSON_URI, " error: ", error);
                alert("Error: " + error);
            });
            console.log("idSaveConfig_OnClick");
        }
	}
})
