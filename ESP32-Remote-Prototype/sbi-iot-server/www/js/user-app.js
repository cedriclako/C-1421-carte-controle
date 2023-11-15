var mData = 
{
    sysinfos: [],
    server_infos_table: [],
    
    pairingsettings: { mac_addr : "" },
    wifisettings: { en: false, ssid: '', pass: '' },

    ota: null,

    livedata: { update_process: { perc: 0, statustext: "" } },

    idSavePairingSettings_IsDisabled : false,
    idSaveWiFiSettings_IsDisabled : false,
    idCheckOTAAvailability_IsDisabled : false,
};

function showTab(evt, cityName) {
    // Declare all variables
    var i, tabcontent, tablinks;

    // Get all elements with class="tabcontent" and hide them
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].style.display = "none";
    }

    // Get all elements with class="tablinks" and remove the class "active"
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].className = tablinks[i].className.replace(" active", "");
    }

    // Show the current tab, and add an "active" class to the button that opened the tab
    document.getElementById(cityName).style.display = "block";
    evt.currentTarget.className += " active";
}

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
        postAction(url, data, onCallback) {
            var reponseStatus = 0;
            fetch(url, {
                method: 'POST',
                headers: {
                  'Content-Type': 'application/json'
                },
                body: data,
                cache: 'default',
                keepalive: false
              })
              .then((response) => 
              {
                // Follow redirection ...
                if (response.redirected) {
                    window.location.href = response.url;
                }
                // Get response within two promises ....
                reponseStatus = response.status;
                return response.text();
              })
              .then(text =>
                {
                    if (reponseStatus != 200) {
                        throw new Error(text);
                    } else {
                        if (onCallback && onCallback.onSuccess) {
                            onCallback.onSuccess(text);
                        }
                    }    
                })
              .catch((ex) =>
              {
                console.error('url: ', url, " error: ", ex);
                if (onCallback && onCallback.onError) {
                    onCallback.onError(ex);
                }
              });
        },
        page_loaded() {
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
        
            // Get system informations
            fetch(API_GETPOST_PAIRINGSETTINGS, { keepalive: false })
                .then((response) => response.json())
                .then((data) => this.pairingsettings = data)
                .catch((ex) =>
                {
                    console.error('API_GETPOST_PAIRINGSETTINGS', ex);
                });
            // Get Wi-Fi informations
            fetch(API_GETPOSTWIFISETTING, { keepalive: false })
                .then((response) => response.json())
                .then((data) => this.wifisettings = data)
                .catch((ex) =>
                {
                    console.error('API_GETPOSTWIFISETTING', ex);
                });

            setTimeout(this.automaticUpdate, 500);
        },
        idBtTroubleshoot_Click() {
            let password = prompt("Please enter your password", "");
            if (password != null) {
                this.postAction(
                    API_POST_ACCESSMAINTENANCEREDIRECT_URI,
                    JSON.stringify({ password: password }),
                    {
                        onError: e => { 
                            alert(e); 
                        },
                        onSuccess: () => { /* If it succeed, it will automatically redirect */}
                    });
            }
        },
        idBtReboot_Click() {
            this.postAction(
                API_ACTION_REBOOT_URI,
                JSON.stringify({ }),
                {
                    onError: e => { 
                        alert(e); 
                    },
                    onSuccess: () => { alert("The system will reboot, it will take a few seconds to go back online."); }
                });
        },  
        idCheckOTAAvailability_Click() {
            this.idCheckOTAAvailability_IsDisabled = true;
            this.ota = null;

            this.postAction(
                API_ACTION_CHECKOTAAVAILABILITY_URI,
                JSON.stringify({ }),
                {
                    onError: e => { 
                        alert(e);
                        this.idCheckOTAAvailability_IsDisabled = false;
                    },
                    onSuccess: () => { 
                        fetch(API_GETOTALIST, { keepalive: false })
                        .then((response) => response.json())
                        .then((data) => {
                            this.ota = data;
                            console.log(API_GETOTALIST, data);
                        }
                        )
                        .catch((ex) =>
                        {
                            alert('API_GETOTALIST', ex);
                        });
                        this.idCheckOTAAvailability_IsDisabled = false; 
                    }
                });
        },
        idOTAInstall_Click(e) {
            console.log("idOTAInstall_Click", e);            
            this.postAction(
                API_INSTALLOTA,
                JSON.stringify({ id: e.id }),
                {
                    onError: e => { 
                        alert(e);
                    },
                    onSuccess: () => { 
                        /* Status report will handle the rest */
                    }
                });
        },
        idSavePairingSettings_Click() {
            this.idSavePairingSettings_IsDisabled = true;
            this.postAction(API_GETPOST_PAIRINGSETTINGS, JSON.stringify(this.pairingsettings),
            {
                onError: e => { 
                    alert(e); 
                    this.idSavePairingSettings_IsDisabled = false; 
                },
                onSuccess: () => { 
                    alert("Success !"); 
                    this.idSavePairingSettings_IsDisabled = false; 
                }
            });
        },
        idBtSaveWiFiSettings_Click() {
            this.idSaveWiFiSettings_IsDisabled = true;
            this.postAction(API_GETPOSTWIFISETTING, JSON.stringify(this.wifisettings),
            {
                onError: e => { 
                    alert(e); 
                    this.idSaveWiFiSettings_IsDisabled = false; 
                },
                onSuccess: () => { 
                    alert("Success !"); 
                    this.idSaveWiFiSettings_IsDisabled = false; 
                }
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
                        // Server infos tables
                        this.server_infos_table = this.json2Table(this.livedata.stove.server);

                        setTimeout(this.automaticUpdate, 1000);
                    })
                .catch((ex) =>
                {
                    console.error('automaticUpdate', ex);
                    setTimeout(this.automaticUpdate, 5000);
                });
        }
	}
})
