function gotoTroubleshoot() {
    let password = prompt("Please your password", "");
    if (password != null) {
        const url = '/api/access-maintenance-redirect?password=' + encodeURIComponent(password);
        window.location.href = url;
    }
}
