# NNProcessor
Учебный проект, который моделирует работу нейронного процессора самостоятельно разработанной архитектуры. 

## Конфигурация
После подачи питания контроллер ввода-вывода загружает в общую память структуру сети, конфигурацию ядер и данные для рассчета первого изображения, после чего создает прерывание IConfig. Получив данное прерывание, ядра начинают читать структуру и свою конфигурацию и производят вычисление первого слоя.

## Расчет выхода сети
Ядро, которое последним записывает результаты вычисления слоя в общую память, генерирует прерывание ILayer. По этому прерыванию начинается вычисление последующего слоя.
Алгоритм повторяется до тех пор, пока при завершении вычисления слоя поле в общей памяти, хранящее счетчик количества пропитанных слоев, не станет равным с полем, конфигурации сети, хранящим количество слоев сети. В случае равенства ядро генерирует прерывание IFinish, которое дает сигнал контроллеру ввода-вывода о готовности результата. 
Затем контроллер ввода-вывода производит выгрузку результата и загружает данные для следующего расчета. После загрузки он генерирует прерывание ILayer, которое запускает процесс расчета сети. 

Для сборки требуется библиотека HDF5.