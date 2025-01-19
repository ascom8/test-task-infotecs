# Тестовое задание для стажера на позицию "Программист на языке C++" в компанию Infotecs

 В качестве тестового задания было предложено реализовать две программы. Взаимодействие программ должно было быть реализовано через использование linux-сокетов.

## Программа 1
Должна состоять из двух потоков и одного общего буфера. 

Поток 1.  Принимает строку, которую введет пользователь.  Должна быть проверка, что строка состоит только из буквенных символов и не превышает 64 символа. После проверки необходимо подсчитать все вхождения каждого символа в исходную последовательность. 
Результат поиска необходимо передать в общий буфер и поток должен ожидать дальнейшего ввода пользователя.

Поток 2. Должен обрабатывать данные, которые помещаются в общий буфер. После получения данных общий буфер затирается. Поток должен вывести полученные данные на экран и  отправить в Программу №2. После этого поток ожидает следующие данные.

## Программа 2
Ожидает данные от  Программы №1. При получении  данных, происходит анализ преданной строки и выводится в консоль все символы в порядке возрастания количества этих символов. Далее программа продолжает ожидать данные.

## Сборка 
Исходный код компилируется средствами cmake с использованием gcc для работы в среде Linux.

Процесс сборки и запуска:
* Выполнить клонирование репозитория с помощью команды `git clone https://github.com/ascom8/test-task-infotecs`;
* В папке репозитория создать папку build и перейти в неё;
* Запустить процесс сборки с помощью команд `cmake ..` и `make`;
* После завершения сборки запустить соответсвующие программы, расположенные в папке build `./program1server/server` и `./program2client/client`.
