import ScatterJS from 'scatterjs-core';
import ScatterEOS from 'scatterjs-plugin-eosjs2';
import { Api, JsonRpc, JsSignatureProvider } from "eosjs";

const endpoint = "https://jungle2.cryptolions.io:443";
const network = {
    blockchain:'eos',
    protocol:'https',
    host:'jungle2.cryptolions.io',
    port: 443,
    chainId:'e70aaab8997e1dfce58fbfac80cbbb8fecec7b99cf982a9444273cbc64c41473'
}

var EOSInstance;

// =============== SCATTER / EOS FUNCTIONS =====================>
class EOSHandler {
    constructor(app_name, contract_account) {
        this.DappName = app_name;
        this.ContractAccount = contract_account;
        this.initialized = false;
        ScatterJS.plugins(new ScatterEOS());
        console.log("Initialized");
    }

    async Connect() {
        console.log("Trying to connect...");
        ScatterJS.scatter.connect(this.DappName).then(connected => {
            if (!connected) {
                console.log("Failed to connect with Scatter!");
                return;
            }

            this.scatter = ScatterJS.scatter;

            this.requiredFields = {
                accounts: [network]
            };

            this.scatter.getIdentity(this.requiredFields).then(() => {
                this.account = this.scatter.identity.accounts.find(
                    x => x.blockchain === 'eos'
                );

                if (this.account == null)
                    return;

                this.rpc = new JsonRpc(endpoint);
                this.api = new Api({ rpc: this.rpc, signatureProvider: this.scatter.eosHook(network)});
                this.initialized = true;

                window.ScatterJS = null;
                console.log("Connected Successfully");
                $("#action-connect").addClass("disabled");
                $("#action-connect").addClass("disabled-link");
                $(".balance-box").show();
                EOSInstance.Update();
            }).catch(error => {
                console.error(error);
            });
        });
    }

    async Register() {
        var isAccountAlreadyRegistered = false;

        var result_mining = await this.rpc.get_table_rows({
            json: true,
            code: 'hytalecontra',
            scope: 'hytalecontra',
            table: 'reputation',
            limit: 9999,
        }).then((result) => {
            for(var i = 0; i < result.rows.length; i++) {
                if(result.rows[i].user == this.account.name) {
                    isAccountAlreadyRegistered = true;
                    break;
                }
            }
        });

        if (isAccountAlreadyRegistered) {
            display_information("Error", "Account is already registered.");
            return;
        }

        try {
            var result = await this.api.transact({
                actions: [{
                    account: this.ContractAccount,
                    name: 'reguser',
                    authorization: [{
                        actor: this.account.name,
                        permission: this.account.authority,
                    }],
                    data: {
                            user: this.account.name,
                            memo: `Registered ${this.account.name} for Mining`,
                    },
                }]
            }, {
                blocksBehind: 3,
                expireSeconds: 30,
            });

            display_information("Success", "Account was registered successfully.");
        } catch(error) {
            display_information("Error", "Account is already registered.");
        }
    }
    
    async Mine(to, quantity) {
        try {
            var result = await this.api.transact({
                actions: [{
                    account: this.ContractAccount,
                    name: 'mine',
                    authorization: [{
                        actor: this.account.name,
                        permission: this.account.authority,
                    }],
                    data: {
                            user_from: this.account.name,
                            user_to: to,
                            quantity: `${quantity} HYTALE`,
                            memo: `User ${this.account.name} awarded ${to} ${quantity} HYTALE`,
                    },
                }]
            }, {
                blocksBehind: 3,
                expireSeconds: 30,
            });

            display_information("Success", `Tokens were successfully mined and sent to: ${to}`);
            EOSInstance.Update();
        } catch(error) {
            display_information("Failed", `Either not enough mineable tokens available, the account doesn't exist, or something else.`);
        }
    }

    async Transfer(user_to, quantity) {
        try {
            var result = await this.api.transact({
                actions: [{
                    account: "hytaletokens",
                    name: 'transfer',
                    authorization: [{
                        actor: this.account.name,
                        permission: this.account.authority,
                    }],
                    data: {
                            from: this.account.name,
                            to: user_to,
                            quantity: `${quantity} HYTALE`,
                            memo: `User ${this.account.name} transferred ${quantity} HYTALE to ${user_to}`,
                    },
                }]
            }, {
                blocksBehind: 3,
                expireSeconds: 30,
            });

            display_information("Success", `User ${this.account.name} transferred ${quantity} HYTALE to ${user_to}`);
            EOSInstance.Update();
        } catch (error) {
            display_information("Error", `Either incorrect balance available or username does not exist.`);
        }
    }

    async Update() {
        var result_tokens = await this.rpc.get_table_rows({
            json: true,
            code: 'hytaletokens',
            scope: this.account.name,
            table: 'accounts',
            limit: 1,
        }).then((result_tokens) => {
            if (result_tokens.rows.length > 0) {
                var balance_sheet = `Balance: ${result_tokens.rows[0].balance}`;
                $("#balance").text(`${balance_sheet}`);
            }
        });

        var result_mining = await this.rpc.get_table_rows({
            json: true,
            code: 'hytalecontra',
            scope: 'hytalecontra',
            table: 'reputation',
            limit: 9999,
        }).then((result_mining) => {
            for (var i = 0; i < result_mining.rows.length; i++) {
                if (result_mining.rows[i].user == this.account.name) {
                    var mining = `Mineable: ${result_mining.rows[i].rep_left_for_day}`;
                    var nextclaimtime = `Time Left Until Claim: ${result_mining.rows[i].next_claim_time}`;
                    $("#mineable").text(`${mining}`);
                    $("#claimtime").text(`${nextclaimtime}`);
                    break;
                }
            }
        });

        $("#action-register").prop("disabled","true");
    }
}

function IsScatterInitialized() {
    if (EOSInstance === undefined) {
        display_information("Error", "Must connect with Scatter first.");
        return false;
    }

    if (!EOSInstance.initialized) {
        display_information("Error", "Scatter has not been initialized yet.");
        return false;
    }

    return true;
}

$("#action-connect").click(function() {
    if (EOSInstance !== undefined)
        return;

    EOSInstance = new EOSHandler("HytaleReputation", "hytalecontra");
    EOSInstance.Connect();
});

$("#action-register").click(function() {
    if (!IsScatterInitialized())
        return;

    EOSInstance.Register();
});

$("#action-mine").click(function() {
    if (!IsScatterInitialized())
        return;
    
    var account_name = $("#input-mining").val().toLowerCase();

    if (account_name == undefined) {
        display_information("Error", "Account Name is undefined.");
        return;
    }
    
    if (account_name.length > 12 || account_name.length <= 0) {
        display_information("Error", "Account Name provided must be less than or equal to 12 characters and greater than 0 characters.");
        return;
    }

    if (account_name == EOSInstance.account.name) {
        display_information("Error", "Cannot send to self.");
        return;
    }

    var quantity = $("#input-mining-quantity").val();

    if (quantity <= 0) {
        display_information("Error", "Quantity must be greater than or equal to: 0.0001");
        return;
    }

    EOSInstance.Mine(account_name, quantity);
});

$("#action-transfer").click(function() {
    if (!IsScatterInitialized)
        return;

    var account_name = $("#input-transfer-to").val().toLowerCase();

    if (account_name == undefined) {
        display_information("Error", "Account Name is undefined.");
        return;
    }
    
    if (account_name.length > 12 || account_name.length <= 0) {
        display_information("Error", "Account Name provided must be less than or equal to 12 characters and greater than 0 characters.");
        return;
    }

    if (account_name == EOSInstance.account.name) {
        display_information("Error", "Cannot send to self.");
        return;
    }

    var quantity = $("#input-transfer-quantity").val();

    if (quantity <= 0) {
        display_information("Error", "Quantity must be greater than or equal to: 0.0001");
        return;
    }

    EOSInstance.Transfer(account_name, quantity);
});

window.GetBalance = async function GetBalance() {
    if (!IsScatterInitialized())
        return;

    var account_name = $("#text-input").val();

    if (account_name.length >= 13) {
        $(".results").append(`<p>Account name is too long. Must be less than 12 characters.</p>`);
        return;
    }

    EOSInstance.GetBalance(account_name);
}

// =============== WEB FUNCTIONS =====================>
const content_div_ids   = ["content-about", "content-mine", "content-transfer", "content-servers"];
const default_link      = `link-about`;
const default_content   = `${content_div_ids[0]}`;
const active_link_class = "active-link"

var last_selected_link = "link-about";

$(document).ready(function() {
    set_active_content(default_link, default_content);
    $(".balance-box").hide();
});

$(".nav-link").click(function() {
    var link_div_id = $(this).attr("id"); // Get the ID

    if (link_div_id == "action-connect")
        return;

    var content_div = link_div_id.replace("link", "content"); // Get Corresponding Content DIV.
    set_active_content(link_div_id, content_div);
});

function set_active_content(link_div, content_div) {
    hide_all_content();
    $(`#${last_selected_link}`).removeClass(active_link_class);
    $(`#${link_div}`).addClass(active_link_class);
    $(`#${content_div}`).show();

    last_selected_link = link_div;
}

function hide_all_content() {
    for(var i = 0; i < content_div_ids.length; i++) {
        $(`#${content_div_ids[i]}`).hide();
    }
}

function display_information(title, message) {
    $('#modal-title').text(title);
    $('#modal-text').text(message);
    $('#info-modal').modal('show');
}