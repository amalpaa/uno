var ready = false;
var game_started = false;
var p_id;
var card_on_table = 255;
var turns_till_yours = 255;
var starting_turns_num = 0;
var connected_sessions = 0;
var turns_direction = -1; // 1 if reversed
var is_fresh = false;

if ('WebSocket' in window) {
    var ws = new WebSocket('ws://127.0.0.1:8080/websocket', "uno-protocol");
    ws.onopen = () => {
        console.log('websocket success---');
    }
    ws.onmessage = (message) => {
        let data = String(message.data);
        let count;
        switch(data[0]) {
            case "T":
                ws.send("T");
                break;
            case "m":
                RenderMessage(data);
                break;
            case "M": {
                let message_space = document.createElement("p1");
                message_space.append(document.createTextNode(data.substring(1)));
                document.getElementById("messages").append(message_space);
                document.getElementById("messages").innerHTML += "<br>";
                break; }
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
                break;
            case "i":
                p_id = ToCInt(data.slice(1, 5));
                break;
            case "w": {
                let player = GetCurrentPlayer();
                card_on_table = data.charCodeAt(1)-1;
                PlayCardAnimation(player.getElementsByClassName("card-icon")[0]);
                count = player.getElementsByTagName("p1")[0];
                count.innerHTML = Number(count.innerHTML.slice(0, -1))-1+"x";
                if(card_on_table >= 40) {
                    if(card_on_table < 44) {
                        turns_direction *= -1;
                    } else if(card_on_table < 48) {
                        NextTurn();
                    } else if(card_on_table < 52) {
                        is_fresh = true;
                    }
                }
                NextTurn();
                break; }
            case "d": {
                let player = GetCurrentPlayer();
                let amount = data.charCodeAt(1);
                DrawCardAnimation(player.getElementsByClassName("card-icon")[0], amount);

                is_fresh = false;
                count = player.getElementsByTagName("p1")[0];
                count.innerHTML = Number(count.innerHTML.slice(0, -1))+amount+"x";
                NextTurn();
                break; }
            case "u": {
                let new_cards_amount = data.charCodeAt(1);
                for(i=2; i<new_cards_amount+2; i++) AppendCard(data.charCodeAt(i)-1);
                break; }
            case "W": 
                card_on_table = data.charCodeAt(1)-1;
                RenderCard(document.getElementById("table-card"), card_on_table);
                PrintWinner();
            case "R": 
                turns_till_yours = 255;
                connected_sessions = 0;
                game_started = false;
                ready = false;
                document.getElementById("other-players").innerHTML = "";
                document.getElementById("ready").style.backgroundColor = "rgb(250, 125, 93)";
                document.getElementById("table-text-space").innerText = "";
                document.getElementById("cards-row").innerHTML = "";
                break;
            case "q": {
                let id = ToCInt(data.slice(1, 5));
                Array.from(document.getElementsByClassName("player")).forEach((player) => {
                    if(player.data == id) {
                        player.getElementsByTagName("p1")[0].textContent = data.slice(5, 21);
                        return;
                    }
                })
                break; }
            case "c": {
                let id = ToCInt(data.slice(1, 5));
                AppendPlayer(data.slice(5, 21), id, 7, data[21].charCodeAt(0), id != p_id);
                break; }
        }

        console.log('get websocket message---', data);
    }
    ws.onerror = () => {
        console.error('websocket fail');
    }
} else {
    console.error('dont support websocket');
};

function RenderMessage(message) {
    let messages = document.getElementById("messages");
    let message_con = document.createElement("p1");
    let player = document.createElement("span");

    player.style.color = "green"; 
    player.style.marginRight = "5px";
    player.append(document.createTextNode(message.slice(2, 1+message.charCodeAt(1))));

    message_con.append(player);
    message_con.append(document.createTextNode(message.slice(1+message.charCodeAt(1))));
    messages.append(message_con);
    messages.innerHTML += "<br>";
}

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

    if(card_on_table <= 52) {
        if(card <= 52) {
            if(t_number != number) {
                if(t_color != color) {
                    PrintErrorToUser("Cards have different number and color");
                    return;
                } else if(is_fresh && card_on_table > 48) {
                    PrintErrorToUser("Can only play plus card");
                    return;
                }
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
    let color = GetCardColor(card);
    if(card < 40) {
        let number = Math.floor(card / 4);
        node.style.backgroundImage = "url(resource/" + color + number + ".png)";
    } else if (card < 44) {
        node.style.backgroundImage = "url(resource/" + color + "r.png)";
    } else if (card < 48) {
        node.style.backgroundImage = "url(resource/" + color + "b.png)";
    } else if (card < 52) {
        node.style.backgroundImage = "url(resource/" + color + "+2.png)";
    }
}

function PrintErrorToUser(message) {
    document.getElementById("messages").innerHTML += "<p1 style=\"color:red\">- " + message + "</p1><br>";
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

function ChangeUsername() {
    let username = document.getElementById("_username").value;
    if(username.length < 6) return;
    document.getElementById("_username").value = "";
    ws.send("q" + username);
}

function DrawCardAnimation(player_card_obj, amount) {
    let rect = document.getElementById("draw-pool").getBoundingClientRect();
    let card = document.createElement("div");
    card.classList.add("animation-card");

    card.style.left = (rect.left + scrollX) + "px";
    card.style.top = (rect.top + scrollY) + "px";

    document.body.append(card);
    requestAnimationFrame(() => {
        let rect = player_card_obj.getBoundingClientRect();
        card.style.left = (rect.left + scrollX) + "px";
        card.style.top = (rect.top + scrollY) + "px";
        card.style.height = "50px";
    });
    
    setTimeout(() => card.remove(), 300);
    if(amount > 1) setTimeout(() => DrawCardAnimation(player_card_obj, amount-1), 100);
}

function PlayCardAnimation(player_card_obj) {
    let rect = player_card_obj.getBoundingClientRect();
    let card = document.createElement("div");
    card.classList.add("rev-animation-card");

    card.style.left = (rect.left + scrollX) + "px";
    card.style.top = (rect.top + scrollY) + "px";
    RenderCard(card, card_on_table);

    document.body.append(card);
    requestAnimationFrame(() => {
        let rect = document.getElementById("table-card").getBoundingClientRect();
        card.style.left = (rect.left + scrollX) + "px";
        card.style.top = (rect.top + scrollY) + "px";
        card.style.height = "100px";
    });
    
    setTimeout(() => {
        card.remove();
        RenderCard(document.getElementById("table-card"), card_on_table);
    }, 300);
}