
const API_GETSYSINFO = '/api/getsysinfo';
const API_GETLIVEDATA = '/api/getlivedata';

let mData = 
{
    sysinfos: [],
    livedata: { state: { is_pairing: false}, wireless: { rx: 0,tx: 0, channel: 0}}
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
            console.log("idStartPairing_Click");

            let url = "";
            if (!this.livedata.state.is_pairing) {
                url = 'action/espnow_startpairing';
            }
            else {
                url = 'action/espnow_stoppairing';
            }

            fetch(url, {
                method: 'POST',
                headers: {
                  'Content-Type': 'application/json'
                },
                body: { test: "coucou" },
                cache: 'default'
              })
              .catch((ex) => 
              {
                  console.error('getlivedata', ex);
                  this.sysinfos = null;
              });
        }       
	}
})
