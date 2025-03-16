var ready = false;
var game_started = false;
var p_id;
var card_on_table = 255;
var turns_till_yours = 255;
var starting_turns_num = 0;
var connected_sessions = 0;
var turns_direction = -1; // 1 if reversed

if ('WebSocket' in window) {
    var ws = new WebSocket('ws://127.0.0.1:8080/websocket', "uno-protocol");
    ws.onopen = () => {
        console.log('websocket success---');
    }
    ws.onmessage = (message) => {
        let data = String(message.data);
        let count;
        switch(data[0]) {
            case "m":
                document.getElementById("messages").innerHTML += "<p1><a style=\"color: green\">" + 
                    data.slice(1, 16) + "</a> " + data.slice(16,data.length) + "</p1><br>";
                break;
            case "M":
                document.getElementById("messages").innerHTML += "<p1>" + data.substring(1) + "</p1><br>";
                break;
            case "s":
                card_on_table = data.charCodeAt(1)-1;
                RenderCard(document.getElementById("table-card"), card_on_table);
                ws.send("s");
                break;
            case "p":
                for(i = 1; i < 8; i++) {
                    AppendCard(data.charCodeAt(i)-1);
                }
                turns_till_yours = ToCInt(data.slice(8, 12));
                starting_turns_num = turns_till_yours;
                connected_sessions = ToCInt(data.slice(12, 16));
                turns_till_yours++; // negate effect for start of the game
                break;
            case "P":
                game_started = true;
                turns_direction = -1;
                NextTurn();
            case "c":
                let id = ToCInt(data.slice(1, 5));
                AppendPlayer(data.slice(5, 21), id, 7, data[21].charCodeAt(0), id != p_id);
                break;
            case "i":
                p_id = ToCInt(data.slice(1, 5));
                break;
            case "w":    
                card_on_table = data.charCodeAt(1)-1;
                RenderCard(document.getElementById("table-card"), card_on_table);
                count = GetCurrentPlayer().getElementsByTagName("p1")[0];
                count.innerHTML = Number(count.innerHTML.slice(0, -1))-1+"x";
                NextTurn();
                break;
            case "d":
                count = GetCurrentPlayer().getElementsByTagName("p1")[0];
                count.innerHTML = Number(count.innerHTML.slice(0, -1))+data.charCodeAt(1)+"x";
                NextTurn();
                break;
            case "u":
                let new_cards_amount = data.charCodeAt(1);
                for(i=2; i<new_cards_amount+2; i++) AppendCard(data.charCodeAt(i)-1);
                break;
            case "W": 
                card_on_table = data.charCodeAt(1)-1;
                RenderCard(document.getElementById("table-card"), card_on_table);
                PrintWinner();
                turns_till_yours = 255;
                connected_sessions = 0;
                game_started = false;
                ready = false;
                document.getElementById("other-players").innerHTML = "";
                document.getElementById("ready").style.backgroundColor = "rgb(250, 125, 93)";
                document.getElementById("table-text-space").innerText = "";
                document.getElementById("cards-row").innerHTML = "";
        }

        console.log('get websocket message---', data);
    }
    ws.onerror = () => {
        console.error('websocket fail');
    }
} else {
    console.error('dont support websocket');
};

function SendToWs() {
    ws.send("m" + document.getElementById("_input").value);
    document.getElementById("_input").value = "";
}

function NextTurn() {
    GetCurrentPlayerBg().style.filter = "brightness(100%)";
    turns_till_yours += turns_direction;
    if(turns_till_yours < 0) turns_till_yours += connected_sessions;
    else if (turns_till_yours >= connected_sessions) turns_till_yours -= connected_sessions;
    GetCurrentPlayerBg().style.filter = "brightness(150%)";

    let text_div = document.getElementById("table-text-space");
    if(turns_till_yours==0) {
        if(!text_div.hasChildNodes()) text_div.innerText = "your turn";
    } else if(text_div.hasChildNodes()){
        text_div.innerText = "";
    }
}

function Ready() {
    if(game_started) {
        PrintErrorToUser("Game has already started");
        return;
    }
    if(ready) document.getElementById("ready").style.backgroundColor = "rgb(250, 125, 93)";
    else document.getElementById("ready").style.backgroundColor = "rgb(150 200 150)";
    ready = !ready;

    ws.send("r");
}

function AppendCard(card_id) {
    let card = document.createElement("div");
    card.data = card_id;
    card.classList.add("card");
    card.setAttribute("onclick",`PlayCard(this)`);
    RenderCard(card, card_id);
    document.getElementById("cards-row").append(card);
}

function AppendPlayer(username, player_id, card_count, profile, is_you) {
    let player = document.createElement("div");
    if(!is_you) player.style.backgroundColor = "rgb(192, 146, 18)";
    player.data = player_id;
    player.classList.add("player");
    
    let username_box = document.createElement("p1");
    username_box.append(document.createTextNode(username));

    let player_profile = document.createElement("div");
    player_profile.classList.add("player-img");
    player_profile.style.backgroundImage = "url(resource/player" + profile + ".png)";

    let card_count_box = document.createElement("div");
    card_count_box.classList.add("card-count")

    let card_count_image = document.createElement("div");
    card_count_image.classList.add("card-icon");

    let card_count_text = document.createElement("p1");
    card_count_text.append(document.createTextNode(card_count + "x"));

    card_count_box.append(card_count_image);
    card_count_box.append(card_count_text);

    player.append(username_box);
    player.append(player_profile);
    player.append(card_count_box);
    document.getElementById("other-players").append(player);
}

function PlayCard(card_obj) {
    if(turns_till_yours != 0) {
        PrintErrorToUser("Not your turn");
        return;
    } 
    let card = card_obj.data;
    let t_color = GetCardColor(card_on_table);
    let t_number = Math.floor(card_on_table / 4);
    let color = GetCardColor(card);
    let number =  Math.floor(card / 4);

    if(card_on_table <= 40) {
        if(card <= 40) {
            if(t_number != number && t_color != color) {
                PrintErrorToUser("Cards have different number and color");
                return;
            }
        }
    }

    card_obj.remove();
    ws.send("p" + String.fromCharCode(card));
}

function ToCInt(inputChar) {
    return ( inputChar.charCodeAt(0))
         + ( inputChar.charCodeAt(1) << 8 )
         + ( inputChar.charCodeAt(2) << 16 )
         + ( inputChar.charCodeAt(3) << 24 );
}

function GetCurrentPlayer() {
    return document.getElementsByClassName("card-count")[(starting_turns_num - turns_till_yours + connected_sessions) % connected_sessions];
}

function GetCurrentPlayerBg() {
    return document.getElementsByClassName("player")[(starting_turns_num - turns_till_yours + connected_sessions) % connected_sessions];
}

function Draw() {
    if(!game_started) {
        PrintErrorToUser("Game has not started");
        return;
    } 
    if(turns_till_yours != 0) {
        PrintErrorToUser("Not your turn");
        return;
    }
    ws.send("d");
}

function RenderCard(node, card) {
    if(card <= 40) {
        let number = Math.floor(card / 4);
        let color = GetCardColor(card);
        node.style.backgroundImage = "url(resource/" + color + number + ".png)";
    }
}

function PrintErrorToUser(message) {
    document.getElementById("messages").innerHTML += "<p1><a style=\"color:red\">- " + message + "</a></p1><br>";
}

function GetCardColor(card) {
    switch(card % 4) {
        case 0:
            return 'r';
        case 1:
            return 'g';
        case 2:
            return 'b';
        case 3:
            return 'y';
    }
}

function PrintWinner() {
    document.getElementById("messages").innerHTML += "<p1><a style=\"color:green\">player " 
    + GetCurrentPlayerBg().getElementsByTagName("p1")[0].innerText + " won the game</a></p1><br>";
}

function OnBodyLoad() {
    document.getElementById("_input").addEventListener("keydown", function(event) {
        if(event.key === "Enter") SendToWs();
    });
}