# GPU(CUDA) Profanity Crack
## (Version 1.1)
## Файл config.cfg
* ***"gpu_local_size": 64***  - количество используемых ядер в группе
* ***"folder_to_save_sort_results": "D:/tables_bin_sort"***  - папка с таблицами публичных ключей алгоритма profanity. *(таблицы сгенерированы программой https://github.com/Houzich/OpenCL-Generator-Profanity-All-Public-Keys-Ethereum)*
* ***"folder_to_save_sort_8_bytes": "D:/tables_bin_8_bytes"***  - папка с таблицами публичных ключей алгоритма profanity, первые 8 байт координаты X.*(таблицы сгенерированы программой https://github.com/Houzich/OpenCL-Generator-Profanity-All-Public-Keys-Ethereum)*

## Описание
В начале программы, считываются настройки из файла config.cfg. Потом предлагается ввести номер используемой видеокарты и искомый публичный ключ.</br>
Функцией ReadTablesToMemory считываются таблицы в оперативную память из папки "folder_to_save_sort_8_bytes", объем 32 ГБ. Запускается функция crack_public_key().
Вызывается функция crack_init на GPU, в которой создаются 255*16384 публичных ключей, каждый ключ равен "искомый ключ" минус G*2 192*id.</br>
Далее, постоянно вызывается функция crack(), в которой каждый ключ уменьшается на точку G. Все эти ключи выгружаются на CPU, где происходит поиск их по таблицам, по
8-ми байтам. Если найдено совпадение (в консоли появиться надпись "!!!FOUND 8 BYTES!!!"), то идет обращение к таблице "полных" ключей "folder_to_save_sort_results" и по номеру файла и позиции совпавших 8 байтах, сравниваются полностью 64-байтные ключи. Если совпало, то в функции calcKeyFromResult вычисляется искомый приватный ключ. Если нет, вызывается снова функция crack() и так далее.

# Если нашли ключ
В консоли появиться надписи:
> * !!!FOUND!!!</br>
!!!FOUND!!!</br>
!!!FOUND!!!</br>
!!!FOUND!!!</br>
ROUND: 1256</br>
PUBLIC KEY: 7CEFE04DDBDB17E3861EC995D515BAC16CC2766CCA1D66C27ACDCEE876FB3CD2D811C410835D71C56FAB7E492084A3949AA6797AEFB38AB4B1AB1DD1B6E15F45</br>
FOUND PRIVATE KEY: C8505C6C876399185B499F3C1AE43E5B553496E135DBCC2CA67C4B278CD9BB18</br>
FOUND PRIVATE KEY: C8505C6C876399185B499F3C1AE43E5B553496E135DBCC2CA67C4B278CD9BB18</br>
FOUND PRIVATE KEY: C8505C6C876399185B499F3C1AE43E5B553496E135DBCC2CA67C4B278CD9BB18
*

## Файл ProfanityCrackV11.exe находится в папке exe


### ОБСУЖДЕНИЕ КОДА: https://t.me/BRUTE_FORCE_CRYPTO_WALLET
