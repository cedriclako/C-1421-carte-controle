var mData = 
{
    pairingsetting: { mac_addr : "" },
    idSavePairingSetting_IsDisabled : false
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
        page_loaded()
        {
            console.log("page_loaded");
            // Get system informations
            fetch(API_GETPAIRINGSETTING, { keepalive: false })
                .then((response) => response.json())
                .then((data) => this.pairingsetting = data)
                .catch((ex) =>
                {
                    console.error('API_GETPAIRINGSETTING', ex);
                });
        },
        idTroubleshoot_Click() {
            let password = prompt("Please your password", "");
            if (password != null) {
                this.postAction(
                    "/api/access-maintenance-redirect",
                    JSON.stringify({ password: password }),
                    {
                        onError: e => { 
                            alert(e); 
                        },
                        onSuccess: () => { /* If it succeed, it will automatically redirect */}
                    });
            }
        },        
        idSavePairingSetting_Click()
        {
            this.idSavePairingSetting_IsDisabled = true;
            this.postAction(API_POSTPAIRINGSETTING, JSON.stringify(this.pairingsetting),
            {
                onError: e => { 
                    alert(e); 
                    this.idSavePairingSetting_IsDisabled = false; 
                },
                onSuccess: () => { 
                    alert("Success !"); 
                    this.idSavePairingSetting_IsDisabled = false; 
                }
            });
        }
	}
})
