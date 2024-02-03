# BpwarasiteTwelegramBwot

BpwarasiteTwelegramBwot is an ESP32 standalone [b-parasite](https://github.com/rbaron/b-parasite) client which allows to log, query and graph readings from the b-parasite plant moisture sensor using a telegram bot. 

It also allows setting a moisture level warning. 

You'll need a bot-token which the Telegram Bot Botfather may grant you ( [official tutorial](https://core.telegram.org/bots/tutorial#obtain-your-bot-token) )

In the future a possible feature might also be working on a local http server for full self-reliance and remote deployment possibilites (though those might benefit from an SD-card more).

Important Note: Just as its creator the bot also barks and pants 🐶


## TODO

- individual per-plant moisture warnings
- local http-server
- setup wizard for wifi
- setup wizard for plants
  - editing plant names, warning levels
  - deleting plant sensors
- factory reset feature
