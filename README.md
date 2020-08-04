<p align="center">
  <img width="512" src="https://i.imgur.com/fxNYG03.png">
</p>

Poldek Driver Multiplayer jest częścią [PolMod](https://github.com/ermaccer/PolMod) autorstwa [ermaccer](https://github.com/ermaccer/).

## Użycie

Plik PDMP.sln należy otworzyć Visual Studio 2019. Do debugowania serwera Linux, polecam funkcję [Windows Subsystem for Linux](https://docs.microsoft.com/en-us/windows/wsl/install-win10) z zainstalowanym klientem [SSH](https://www.illuminiastudios.com/dev-diaries/ssh-on-windows-subsystem-for-linux/).

## Ustawienia serwera

Plikiem konfiguracyjnym dla serwera jest server.ini, który powinien znajdować się obok pliku wykonywalnego serwera. Przykładowa zawartość pliku:

```ini
; plik konfiguracyjny serwera Poldek Driver Multiplayer
; pogladowy plik z defaultowymi ustawieniami

[network]		; ustawienia sieciowe
ip = 127.0.0.1		; IPv4
port = 54010		; port domyslny dla PDMP
clients = 40		; maksymalna liczba graczy
logs = true		; zapisywanie logow do pliku
rcon = asdf		; haslo rcon
tickrate = 55		; tickrate serwera

[game]
map = pustynia.mar	; mapa ladowana przy starcie serwera
default_car = pold.mar	; samochod zastepczy w razie bledu
```
## Ustawienia klienta

Plikiem konfiguracyjnym dla klienta jest mp.ini, który powinień znajdować się w katalogu z grą:

```ini
; plik konfiguracyjny klienta Poldek Driver Multiplayer
[player]
nickname = sebol
ip = 127.0.0.1
port = 54010
```
## Znane bugi

- serwer Linux zapętla odbieranie pakietu od rozłączonego użytkownika

## Licencja
[MIT](https://choosealicense.com/licenses/mit/)
