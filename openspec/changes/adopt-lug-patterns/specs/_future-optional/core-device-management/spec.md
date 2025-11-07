# Core Device Management Specification

## ADDED Requirements

### Requirement: Device Ownership Tracking
The system SHALL provide a Device base class that tracks ownership and prevents resource conflicts through reference counting and sharing policies.

#### Scenario: Single device acquired exclusively
- **GIVEN** a Device with SharingPolicy::eSingle
- **WHEN** first caller acquires the device
- **THEN** acquisition SHALL succeed
- **AND** owners_count SHALL be 1
- **WHEN** second caller tries to acquire same device
- **THEN** acquisition SHALL fail with error code device_or_resource_busy
- **AND** owners_count SHALL remain 1

#### Scenario: Shared device acquired by multiple owners
- **GIVEN** a Device with SharingPolicy::eShared
- **WHEN** first caller acquires the device
- **THEN** acquisition SHALL succeed and owners_count SHALL be 1
- **WHEN** second caller acquires same device
- **THEN** acquisition SHALL succeed and owners_count SHALL be 2
- **WHEN** third caller acquires same device
- **THEN** acquisition SHALL succeed and owners_count SHALL be 3

#### Scenario: Device release decrements reference count
- **GIVEN** a Shared device with owners_count = 3
- **WHEN** one owner releases the device
- **THEN** release SHALL succeed
- **AND** owners_count SHALL be 2
- **WHEN** another owner releases
- **THEN** owners_count SHALL be 1
- **WHEN** last owner releases
- **THEN** owners_count SHALL be 0
- **AND** device SHALL be available for new acquisition

### Requirement: Sharing Policy Enforcement
The system SHALL enforce device sharing policies at runtime to prevent misuse of shared resources.

#### Scenario: UART uses Single policy (exclusive access)
- **GIVEN** UART device configured with SharingPolicy::eSingle
- **WHEN** module A acquires UART0
- **THEN** acquisition SHALL succeed
- **WHEN** module B tries to acquire same UART0
- **THEN** acquisition SHALL fail immediately
- **AND** module B SHALL receive clear error about resource being busy

#### Scenario: GPIO pin uses Shared policy (multiple readers)
- **GIVEN** GPIO input pin configured with SharingPolicy::eShared
- **WHEN** sensor A acquires pin for reading
- **THEN** acquisition SHALL succeed
- **WHEN** sensor B also acquires same pin for reading
- **THEN** acquisition SHALL succeed
- **AND** both sensors SHALL be able to read simultaneously

#### Scenario: Policy cannot be changed after construction
- **GIVEN** a Device constructed with SharingPolicy::eSingle
- **WHEN** attempting to change policy to eShared
- **THEN** operation SHALL NOT be possible (const member or no setter)
- **AND** policy SHALL remain eSingle for device lifetime

### Requirement: Device State Management
The system SHALL track device open/close state and enforce proper usage order (acquire → open → use → close → release).

#### Scenario: Cannot use device before opening
- **GIVEN** a Device that has been acquired but not opened
- **WHEN** attempting to perform operations (read/write)
- **THEN** operations SHALL fail with error operation_not_permitted
- **AND** device state SHALL remain closed

#### Scenario: Device lifecycle in correct order
- **GIVEN** a newly created device
- **WHEN** user acquires device
- **THEN** acquisition SHALL succeed and owners_count increments
- **WHEN** user opens device
- **THEN** open SHALL succeed and isOpened() returns true
- **WHEN** user performs operations
- **THEN** operations SHALL succeed
- **WHEN** user closes device
- **THEN** close SHALL succeed and isOpened() returns false
- **WHEN** user releases device
- **THEN** release SHALL succeed and owners_count decrements

#### Scenario: Cannot open already-opened device
- **GIVEN** a Device that is already opened
- **WHEN** attempting to open again
- **THEN** operation SHALL fail with error device_already_opened
- **AND** device state SHALL remain opened (no side effects)

### Requirement: Global Device Registry
The system SHALL provide a centralized registry for managing device instances with type-safe access by device ID.

#### Scenario: Registry initialization with board devices
- **GIVEN** a board configuration (e.g., SAME70 Xplained)
- **WHEN** registry is initialized with board devices
- **THEN** registry SHALL contain all board peripherals
- **AND** each device SHALL be accessible by its DeviceId
- **AND** devices SHALL be stored as shared_ptr for lifetime management

#### Scenario: Type-safe device retrieval
- **GIVEN** a registry containing UART0 device
- **WHEN** calling getDevice<IUart>(DeviceId::eUart0)
- **THEN** SHALL return shared_ptr<IUart> to UART0
- **AND** type SHALL be enforced at compile-time
- **WHEN** calling getDevice<ISpi>(DeviceId::eUart0)
- **THEN** SHALL fail at compile-time or runtime (type mismatch)

#### Scenario: Device not found error
- **GIVEN** a registry that doesn't contain device ID eI2c2
- **WHEN** requesting getDevice<II2c>(DeviceId::eI2c2)
- **THEN** SHALL return nullptr or throw error
- **AND** error SHALL clearly indicate device not found
- **AND** SHALL list valid device IDs in error message (debug builds)

### Requirement: Board-Specific Device IDs
The system SHALL provide board-specific device ID enumerations for type-safe and readable device access.

#### Scenario: SAME70 board device enumeration
- **GIVEN** SAME70 Xplained board configuration
- **WHEN** defining device IDs
- **THEN** SHALL include all board peripherals:
  - eUart0, eUart1, eUart2
  - eLedGreen, eLedRed, eLedBlue
  - eI2c0, eI2c1, eI2c2
  - eSpi0, eSpi1
  - eAdc0, eDac0, ePwm0
- **AND** IDs SHALL be scoped enum (type-safe)
- **AND** IDs SHALL have descriptive names

#### Scenario: Multiple boards in same project
- **GIVEN** project supporting both SAME70 and STM32F4
- **WHEN** each board defines its device IDs
- **THEN** IDs SHALL be in separate namespaces or enum classes
- **AND** SHALL NOT conflict with each other
- **AND** compile-time selection SHALL choose correct board

### Requirement: RAII-Compatible Device Management
The system SHALL integrate with C++ RAII patterns using shared_ptr for automatic lifetime management and cleanup.

#### Scenario: Shared pointer reference counting
- **GIVEN** a device retrieved from registry as shared_ptr
- **WHEN** shared_ptr is copied to another variable
- **THEN** reference count SHALL increase
- **WHEN** one shared_ptr goes out of scope
- **THEN** device SHALL remain valid (other references exist)
- **WHEN** last shared_ptr goes out of scope
- **THEN** device SHALL be released automatically

#### Scenario: Custom deleter for device return
- **GIVEN** a device with custom cleanup requirements
- **WHEN** shared_ptr is destroyed
- **THEN** custom deleter SHALL be invoked
- **AND** device SHALL be properly returned to registry
- **AND** device state SHALL be reset for reuse

#### Scenario: Move semantics avoid extra ref counting
- **GIVEN** a shared_ptr<IUart> device
- **WHEN** device is moved to another function
- **THEN** no reference count increment SHALL occur
- **AND** source pointer SHALL become null
- **AND** destination SHALL take ownership

### Requirement: Thread Safety Considerations
The system SHALL document thread safety guarantees and provide guidance for multi-threaded usage (if RTOS support is enabled).

#### Scenario: Single-threaded usage (default)
- **GIVEN** CoreZero running without RTOS
- **WHEN** devices are acquired and released
- **THEN** no mutex locking SHALL occur
- **AND** operations SHALL have minimal overhead
- **AND** thread safety is user's responsibility

#### Scenario: Multi-threaded usage with RTOS (future)
- **GIVEN** CoreZero with RTOS support enabled
- **WHEN** multiple threads access device registry
- **THEN** registry operations SHALL be mutex-protected
- **AND** acquire/release SHALL be atomic
- **AND** SHALL prevent race conditions

#### Scenario: Device usage thread affinity
- **GIVEN** a device acquired in thread A
- **WHEN** thread B tries to use same device
- **THEN** behavior SHALL be documented (undefined or error)
- **AND** best practice SHALL be one device per thread
- **AND** Shared devices MAY support multi-thread access if documented

### Requirement: Error Reporting and Debugging
The system SHALL provide clear error messages and debugging aids for device management issues.

#### Scenario: Device busy error includes details
- **GIVEN** UART0 acquired by module A
- **WHEN** module B tries to acquire UART0
- **THEN** error SHALL be device_or_resource_busy
- **AND** debug builds SHALL log which module owns device
- **AND** debug builds SHALL log current owners_count

#### Scenario: Device not opened error is clear
- **GIVEN** device used without calling open()
- **WHEN** operation fails
- **THEN** error SHALL be operation_not_permitted
- **AND** error message SHALL state "device not opened"
- **AND** SHALL suggest calling open() first

#### Scenario: Registry provides device listing (debug)
- **GIVEN** development/debug mode enabled
- **WHEN** calling registry.list_devices()
- **THEN** SHALL return list of all registered devices
- **AND** SHALL show device ID, type, and current state
- **AND** SHALL show ownership information

### Requirement: Performance and Memory Efficiency
The system SHALL minimize overhead of device management to be suitable for resource-constrained embedded systems.

#### Scenario: Device base class has minimal size
- **GIVEN** Device base class implementation
- **WHEN** measuring sizeof(Device)
- **THEN** SHALL be <= 32 bytes
- **AND** SHALL contain only essential state (policy, count, opened flag)
- **AND** SHALL use bit-packing if beneficial

#### Scenario: Registry lookup is efficient
- **GIVEN** registry with 20 devices
- **WHEN** looking up device by ID
- **THEN** SHALL use efficient map (e.g., std::unordered_map or array)
- **AND** lookup SHALL be O(1) average case
- **AND** SHALL NOT involve string comparisons

#### Scenario: Acquire/release overhead is minimal
- **GIVEN** device acquire operation
- **WHEN** measuring CPU cycles
- **THEN** SHALL be <100 cycles on typical ARM Cortex-M
- **AND** SHALL be single increment/decrement + comparison
- **AND** no dynamic allocation SHALL occur

### Requirement: Documentation and Usage Examples
The system SHALL provide comprehensive documentation covering device management patterns and best practices.

#### Scenario: Documentation includes ownership patterns
- **GIVEN** developer reading device management docs
- **WHEN** learning about ownership
- **THEN** SHALL find examples of Single vs Shared policies
- **AND** SHALL find guidance on choosing policy
- **AND** SHALL find examples of resource conflict prevention

#### Scenario: Documentation covers registry usage
- **GIVEN** developer setting up new board
- **WHEN** consulting documentation
- **THEN** SHALL find guide to defining device IDs
- **AND** SHALL find guide to initializing registry
- **AND** SHALL find examples of retrieving devices

#### Scenario: Troubleshooting guide exists
- **GIVEN** developer encountering device busy error
- **WHEN** consulting troubleshooting docs
- **THEN** SHALL find explanation of common causes
- **AND** SHALL find debugging techniques
- **AND** SHALL find migration patterns from old code
