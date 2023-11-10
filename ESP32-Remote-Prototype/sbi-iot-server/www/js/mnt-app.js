let mData =
{
    sysinfos: [],
    livedata: {
        state:{
           is_pairing: false
        },
        wireless:{
           rx: 0,
           tx: 0,
           channel: 0
        },
        stove:{
           is_connected: false,
           param_cnt: 0,
           is_param_upload_error: false,
           is_param_download_error: false,
           debug_string: "---",
           server : {
            filecrc32: 0,
            filesize: 0,
            firmwareID : 0,
            version_major : 0,
            version_min : 0,
            version_rev : 0,
            git_commitid: "",
            git_branch: "",
            git_isdirty: false,
           }
        },
        remote:{
           tempC_current: 0,
           tempC_sp: 0,
           fanspeed: 0,
           lastcomm_ms: 0
        },
        datetime: "",
    },
    debug_string_table: [],
    server_infos_table: [],
    // Config JSON
    configJSON: "",
    isStringMode: false,
    configJSONObj: null
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
                cache: 'default',
                keepalive: false
              }).then((data) =>
              {
                console.log("postAction: ", data);
              })
              .catch((ex) =>
              {
                  console.error('url: ', url, " error: ", ex);
              });
        },
        json2Table(obj) {
            let newTable = [];

            const keys = Object.keys(obj);
            const vals = Object.values(obj);

            for (let i = 0; i < keys.length; i++) {
                let newItem = { name: keys[i], value: vals[i] };
                newTable.push(newItem);
            }
            return newTable;
        },

        automaticUpdate() {
            fetch(API_GETLIVEDATA, { keepalive: false })
                .then((response) => response.json())
                .then(
                    (data) =>
                    {
                        this.livedata = data;
                        // --------------------------
                        // Debug tables
                        let debugStringObj = JSON.parse(this.livedata.stove.debug_string);
                        this.debug_string_table = this.json2Table(debugStringObj);
                        // --------------------------
                        // Server infos tables
                        this.server_infos_table = this.json2Table(this.livedata.stove.server);

                        setTimeout(this.automaticUpdate, 500);
                    })
                .catch((ex) =>
                {
                    console.error('automaticUpdate', ex);
                    setTimeout(this.automaticUpdate, 5000);
                });
        },
        page_loaded()
        {
            console.log("page_loaded");
            // Get system informations
            fetch(API_GETSYSINFO, { keepalive: false })
                .then((response) => response.json())
                .then((data) => this.sysinfos = data.infos)
                .catch((ex) =>
                {
                    console.error('getSysInfo', ex);
                    this.sysinfos = null;
                });

            setTimeout(this.automaticUpdate, 500);
        },
        idLoadConfig_OnClick(event)
        {
            console.log("idLoadConfig_OnClick");
            this.configJSON = "";
            this.configJSONObj = null;
            // Get system informations
            let isOK = false;
            fetch(API_GETPOST_SERVERPARAMETERFILEJSON_URI, {
                method: 'GET', // or 'PUT'
                headers: {
                  'Content-Type': 'application/json',
                },
                keepalive: false
              })
                .then((response) =>
                {
                    isOK = response.ok;
                    return response.text();
                })
                .then((response) =>
                {
                    if (!isOK)
                        throw Error(response);
                    console.log("url: ", API_GETPOST_SERVERPARAMETERFILEJSON_URI, " data: ", response);
                    this.configJSON = JSON.stringify(JSON.parse(response), null, 2);
                    this.configJSONObj = JSON.parse(response);
                })
                .catch((error) =>
                {
                    console.error("url: ", API_GETPOST_SERVERPARAMETERFILEJSON_URI, " error: ", error);
                    this.configJSON = error;
                    this.configJSONObj = null;
                });
        },
        idSaveConfig_OnClick(event, bIsTableMode)
        {
            // Table mode
            if (bIsTableMode == 1) {
                this.configJSON = JSON.stringify(this.configJSONObj);
            }
            else {
                this.configJSONObj = JSON.parse(this.configJSON);
            }

            console.log("idSaveConfig_OnClick tablemode: ", bIsTableMode, "config: ", this.configJSON);

            let isOK = false;
            fetch(API_GETPOST_SERVERPARAMETERFILEJSON_URI, {
                method: 'POST', // or 'PUT'
                headers: {
                  'Content-Type': 'application/json',
                },
                body: this.configJSON,
                keepalive: false
              })
            .then((response) =>
            {
                isOK = response.ok;
                return response.text();
            })
            .then((response) => {
                if (!isOK)
                    throw Error(response);
                console.log("url: ", API_GETPOST_SERVERPARAMETERFILEJSON_URI, " data: ", response);
            })
            .catch((error) => {
                console.error("url: ", API_GETPOST_SERVERPARAMETERFILEJSON_URI, " error: ", error);
                alert("Error: " + error);
            });
            console.log("idSaveConfig_OnClick");
        },
        idForceDownloadConfig_Click(event)
        {
            let isOK = false;
            fetch(API_ACTION_DOWNLOADCONFIG_URI, {
                method: 'POST', // or 'PUT'
                headers: {
                  'Content-Type': 'application/json',
                },
                body: "",
                keepalive: false
              })
            .then((response) =>
            {
                isOK = response.ok;
                return response.text();
            })
            .then((response) => {
                if (!isOK)
                    throw Error(response);
                console.log("url: ", API_ACTION_DOWNLOADCONFIG_URI, " data: ", response);
            })
            .catch((error) => {
                console.error("url: ", API_ACTION_DOWNLOADCONFIG_URI, " error: ", error);
                alert("Error: " + error);
            });
            console.log("idForceDownloadConfig_Click");
        }
	}
})
