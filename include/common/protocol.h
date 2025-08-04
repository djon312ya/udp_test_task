/*
 * protocol.h
 *
 *  Created on: Aug 3, 2025
 *      Author: djon
 */
#ifndef _COMMON_UDP_PROTOCOL_H_
#define _COMMON_UDP_PROTOCOL_H_

#include <cstdint>

// Поточна версія протоколу
constexpr uint16_t PROTOCOL_VERSION = 0x0001;

// Розмір даних в одному пакеті (payload). Вибрано для відповідності стандартному MTU.
constexpr size_t DATA_CHUNK_PAYLOAD_SIZE = 1024;

// Кількість чисел типу double для передачі
constexpr size_t TOTAL_DOUBLES = 1000000;

// Типи повідомлень
enum class MessageType : uint8_t {
    REQUEST_DATA = 1,          // Запит від клієнта на отримання даних
    DATA_CHUNK = 2,            // Частина даних від сервера
    ERROR_VERSION_MISMATCH = 3 // Помилка: версія протоколу клієнта застаріла
};

// Заголовок кожного UDP пакету
#pragma pack(push, 1) // Вирівнювання структури для коректної серіалізації
struct PacketHeader {
    uint16_t protocol_version;
    MessageType message_type;
    uint32_t sequence_number; // Номер частини даних
    uint32_t payload_size;    // Розмір даних після заголовка
};
#pragma pack(pop)

// Пейлоад для повідомлення про помилку версії
struct VersionMismatchPayload {
    uint16_t server_version;
};

#endif /*_COMMON_UDP_PROTOCOL_H_*/
