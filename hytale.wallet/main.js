import ScatterJS from 'scatterjs-core';
import ScatterEOS from 'scatterjs-plugin-eosjs2';
import { Api, JsonRpc, JsSignatureProvider } from "eosjs";
import $ from "jquery";

const endpoint = "https://jungle2.cryptolions.io:443";
const network = {
    blockchain:'eos',
    protocol:'https',
    host:'jungle2.cryptolions.io',
    port: 443,
    chainId:'e70aaab8997e1dfce58fbfac80cbbb8fecec7b99cf982a9444273cbc64c41473'
}

var EOSInstance;

class EOSHandler {
    constructor(app_name, contract_account) {
        this.DappName = app_name;
        this.ContractAccount = contract_account;
        this.initialized = false;
        ScatterJS.plugins(new ScatterEOS());
        console.log("Initialized");
    }

    async Connect() {
        console.log("Trying to connect.");
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

                if (this.account == null) {
                    $(".results").append(`<p>Account was null or undefined.</p>`);
                    return;
                }

                this.rpc = new JsonRpc(endpoint);
                this.api = new Api({ rpc: this.rpc, signatureProvider: this.scatter.eosHook(network)});
                this.initialized = true;

                window.ScatterJS = null;
                $(".results").append(`<p>Connected with: ${this.account.name}</p>`);
                console.log("Connected");
            }).catch(error => {
                console.error(error);
            });
        });
    }

    async Register() {
        const result = await this.api.transact({
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


        if (result.transaction_id !== undefined) {
            $(".results").append(`<p>Registered ${this.account.name} for mining.</p>`);
            return;
        }

        console.log(result);
        $(".results").append(`<p>Account ${this.account.name} is already registered.</p>`);
    }
    
    async Mine(to, quantity) {
        const result = await this.api.transact({
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


        if (result.transaction_id !== undefined) {
            $(".results").append(`<p>User ${this.account.name} awarded ${to} ${quantity} HYTALE`);
            return;
        }

        console.log(result);
        $(".results").append(`<p>Mining transaction failed.</p>`);
    }

    async GetBalance(account_name) {
        const result = await this.rpc.get_table_rows({
            json: true,
            code: 'hytaletokens',
            scope: account_name,
            table: 'accounts',
            limit: 10,
        });
    
        if (result.rows[0].balance !== undefined) {
            $(".results").append(`<p>${account_name} has a balance of ${result.rows[0].balance}</p>`);
            return;
        }

         $(".results").append(`<p>Account name does not have balance or result is not available.</p>`);
    }
}

function IsScatterInitialized() {
    if (EOSInstance === undefined) {
        $(".results").append(`<p>Connect with Scatter first</p>`);
        return false;
    }
       

    if (!EOSInstance.initialized) {
        $(".results").append(`<p>Connect with Scatter first.</p>`);
        return false;
    }

    return true;
}


window.OpenScatter = function OpenScatter() {
    if (EOSInstance !== undefined) {
        EOSInstance = undefined;
        $(".results").append(`<p>Disconnected.</p>`);
        return;
    }

    EOSInstance = new EOSHandler("HytaleReputation", "hytalecontra");
    EOSInstance.Connect();
}

window.Register = function Register() {
    if (!IsScatterInitialized())
        return;

    EOSInstance.Register();
}

window.Mine = function Mine() {
    if (!IsScatterInitialized())
        return;
    
    var account_name = $(".text-input").val();
    
    if (account_name.length >= 13) {
        $(".results").append(`<p>Account name is too long. Must be less than 12 characters.</p>`);
        return;
    }

    var quantity = $(".quantity-input").val();

    EOSInstance.Mine(account_name, quantity);
}

window.GetBalance = async function GetBalance() {
    if (!IsScatterInitialized())
        return;

    var account_name = $(".text-input").val();

    if (account_name.length >= 13) {
        $(".results").append(`<p>Account name is too long. Must be less than 12 characters.</p>`);
        return;
    }

    EOSInstance.GetBalance(account_name);
}