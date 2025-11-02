#include "peripheral.hpp"

using alloy::core::u8;
using alloy::core::u16;

/// BLE Peripheral implementation - STUB
///
/// This is a stub implementation for BLE Peripheral (GATT Server).
/// Full implementation will be added in a future phase.
///
/// For now, we're focusing on BLE Central (Scanner) functionality.

namespace alloy::ble {

struct Peripheral::Impl {
    bool initialized;
    bool advertising;

    Impl() : initialized(false), advertising(false) {}
};

Peripheral::Peripheral() : impl_(new Impl()) {}

Peripheral::~Peripheral() {
    if (impl_) {
        deinit();
        delete impl_;
    }
}

Result<void> Peripheral::init(const char* device_name) {
    (void)device_name;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::init(const PeripheralConfig& config) {
    (void)config;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::deinit() {
    return Result<void>::ok();
}

Result<void> Peripheral::start_advertising() {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::start_advertising(const AdvData& adv_data) {
    (void)adv_data;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::stop_advertising() {
    return Result<void>::error(ErrorCode::NotSupported);
}

bool Peripheral::is_advertising() const {
    return impl_->advertising;
}

Result<ServiceHandle> Peripheral::add_service(const UUID& uuid) {
    (void)uuid;
    return Result<ServiceHandle>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::start_service(const ServiceHandle& service) {
    (void)service;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<CharHandle> Peripheral::add_characteristic(
    const ServiceHandle& service,
    const UUID& uuid,
    u8 properties,
    u8 permissions)
{
    (void)service;
    (void)uuid;
    (void)properties;
    (void)permissions;
    return Result<CharHandle>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::set_char_value(
    const CharHandle& characteristic,
    const u8* data,
    u16 length)
{
    (void)characteristic;
    (void)data;
    (void)length;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<u16> Peripheral::get_char_value(
    const CharHandle& characteristic,
    u8* buffer,
    u16 buffer_size) const
{
    (void)characteristic;
    (void)buffer;
    (void)buffer_size;
    return Result<u16>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::notify(
    const CharHandle& characteristic,
    const u8* data,
    u16 length)
{
    (void)characteristic;
    (void)data;
    (void)length;
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::indicate(
    const CharHandle& characteristic,
    const u8* data,
    u16 length)
{
    (void)characteristic;
    (void)data;
    (void)length;
    return Result<void>::error(ErrorCode::NotSupported);
}

bool Peripheral::is_connected() const {
    return false;
}

u8 Peripheral::connection_count() const {
    return 0;
}

Result<void> Peripheral::disconnect_all() {
    return Result<void>::error(ErrorCode::NotSupported);
}

Result<void> Peripheral::disconnect(ConnHandle conn_handle) {
    (void)conn_handle;
    return Result<void>::error(ErrorCode::NotSupported);
}

void Peripheral::set_connection_callback(ConnectionCallback callback) {
    (void)callback;
}

void Peripheral::set_read_callback(ReadCallback callback) {
    (void)callback;
}

void Peripheral::set_write_callback(WriteCallback callback) {
    (void)callback;
}

} // namespace alloy::ble
