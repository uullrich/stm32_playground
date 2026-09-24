#include "CanDispatcher.h"
#include "CustomCan.h"
#include "IoRepository.h"
#include "MockAdcInput.h"
#include "MockCanBus.h"
#include "MockDigitalInput.h"
#include "MockDigitalOutput.h"
#include "MockIoWriteListener.h"
#include "MockPwmOutput.h"
#include "MockVirtualIo.h"
#include "VirtualAdcInput.h"
#include "VirtualDigitalInput.h"
#include "VirtualDigitalOutput.h"
#include "VirtualPwmOutput.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace
{

using uullrich::playground::CUSTOM_CAN_INVALID_IO_FIELD;
using uullrich::playground::CustomCanIoType;
using uullrich::playground::CustomCanNodeId;

constexpr CustomCanNodeId NODE_ID = 0x2A;
constexpr CustomCanNodeId OTHER_NODE_ID = 0x15;
constexpr uint8_t IO_INDEX = 0;
constexpr uint8_t MOCK_IO_INDEX = 1;
constexpr uint8_t UNKNOWN_IO_INDEX = 7;
constexpr uint32_t PWM_PERIOD = 1000;
constexpr uint32_t LARGE_PWM_PERIOD = 4'000'000'000;
constexpr uint16_t ADC_MILLIVOLTS = 1234;
constexpr auto UNKNOWN_IO_TYPE = static_cast<CustomCanIoType>(0x7F);
constexpr auto INVALID_IO_TYPE = static_cast<CustomCanIoType>(CUSTOM_CAN_INVALID_IO_FIELD);

}

namespace uullrich::playground::test
{

using testing::_;
using testing::NiceMock;
using testing::Ref;
using testing::Return;
using testing::StrictMock;

class CanDispatcherTest : public ::testing::Test
{
  protected:
    NiceMock<MockDigitalOutput> m_digitalOutput;
    NiceMock<MockDigitalInput> m_digitalInput;
    NiceMock<MockPwmOutput> m_pwmOutput;
    NiceMock<MockAdcInput> m_adcInput;
    VirtualDigitalOutput m_virtualDigitalOutput{m_digitalOutput, IO_INDEX};
    VirtualDigitalInput m_virtualDigitalInput{m_digitalInput, IO_INDEX};
    VirtualPwmOutput m_virtualPwmOutput{m_pwmOutput, IO_INDEX};
    VirtualAdcInput m_virtualAdcInput{m_adcInput, IO_INDEX};
    IoRepository m_repository;
    NiceMock<MockCanBus> m_canBus;
    StrictMock<MockIoWriteListener> m_writeListener;
    CanDispatcher m_dispatcher{m_canBus, m_repository, m_writeListener, NODE_ID};

    std::vector<CanMessage> m_sent;
    bool m_digitalState{false};
    uint32_t m_pwmPulse{0};

    void SetUp() override
    {
        ASSERT_TRUE(m_repository.add(m_virtualDigitalOutput));
        ASSERT_TRUE(m_repository.add(m_virtualDigitalInput));
        ASSERT_TRUE(m_repository.add(m_virtualPwmOutput));
        ASSERT_TRUE(m_repository.add(m_virtualAdcInput));

        ON_CALL(m_canBus, send(_)).WillByDefault([this](const CanMessage& message) {
            m_sent.push_back(message);
            return ICanBus::Status::Ok;
        });
        ON_CALL(m_digitalOutput, set(_)).WillByDefault([this](bool state) {
            m_digitalState = state;
        });
        ON_CALL(m_digitalOutput, readState()).WillByDefault([this] { return m_digitalState; });
        ON_CALL(m_pwmOutput, period()).WillByDefault(testing::Return(PWM_PERIOD));
        ON_CALL(m_pwmOutput, setPulse(_)).WillByDefault([this](uint32_t pulse) {
            m_pwmPulse = pulse;
        });
        ON_CALL(m_pwmOutput, getPulse()).WillByDefault([this] { return m_pwmPulse; });
        ON_CALL(m_adcInput, readMillivolts())
            .WillByDefault(Return(std::optional<uint16_t>{ADC_MILLIVOLTS}));
    }

    static CanMessage setFrame(CustomCanNodeId node, CustomCanIoType type, uint8_t index,
                               uint32_t value)
    {
        return encodeSetRequest(node, {{type, index}, value});
    }

    static CanMessage getFrame(CustomCanNodeId node, CustomCanIoType type, uint8_t index)
    {
        return encodeGetRequest(node, {{type, index}});
    }

    void expectSingleResponse(CustomCanCommand command, CustomCanStatus status,
                              CustomCanIoType type, uint8_t index, uint32_t value) const
    {
        ASSERT_EQ(m_sent.size(), 1u);
        const CanMessage& message = m_sent.front();
        EXPECT_EQ(message.id, encodeCustomCanId(command, NODE_ID));
        EXPECT_EQ(message.length, 7u);

        const auto response = decodeValueResponse(message);
        ASSERT_TRUE(response);
        EXPECT_EQ(response->status, status);
        EXPECT_EQ(response->io.type, type);
        EXPECT_EQ(response->io.index, index);
        EXPECT_EQ(response->value, value);
    }
};

TEST_F(CanDispatcherTest, SetDigitalOutputDrivesPinAndRespondsOk)
{
    EXPECT_CALL(m_digitalOutput, set(true));
    EXPECT_CALL(m_writeListener, onWritten(Ref(m_virtualDigitalOutput)));

    EXPECT_TRUE(
        m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX, 1)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::DigitalOutput, IO_INDEX, 1);
}

TEST_F(CanDispatcherTest, SetDigitalOutputZeroClearsPin)
{
    m_digitalState = true;
    EXPECT_CALL(m_digitalOutput, set(false));
    EXPECT_CALL(m_writeListener, onWritten(Ref(m_virtualDigitalOutput)));

    EXPECT_TRUE(
        m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX, 0)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::DigitalOutput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetPwmOutputScalesPulseAndRespondsOk)
{
    EXPECT_CALL(m_pwmOutput, setPulse(250));
    EXPECT_CALL(m_writeListener, onWritten(Ref(m_virtualPwmOutput)));

    EXPECT_TRUE(
        m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::PwmOutput, IO_INDEX, 2500)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::PwmOutput, IO_INDEX, 2500);
}

TEST_F(CanDispatcherTest, SetPwmOutputAtMaximumIsAccepted)
{
    EXPECT_CALL(m_pwmOutput, setPulse(PWM_PERIOD));
    EXPECT_CALL(m_writeListener, onWritten(Ref(m_virtualPwmOutput)));

    EXPECT_TRUE(m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::PwmOutput, IO_INDEX,
                                               VirtualPwmOutput::PWM_VALUE_MAX)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::PwmOutput, IO_INDEX, VirtualPwmOutput::PWM_VALUE_MAX);
}

TEST_F(CanDispatcherTest, SetDigitalOutputAboveOneIsValueOutOfRange)
{
    m_digitalState = true;
    EXPECT_CALL(m_digitalOutput, set(_)).Times(0);

    EXPECT_TRUE(
        m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX, 2)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::ValueOutOfRange,
                         CustomCanIoType::DigitalOutput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetPwmOutputAboveMaximumIsValueOutOfRange)
{
    m_pwmPulse = 500;
    EXPECT_CALL(m_pwmOutput, setPulse(_)).Times(0);

    EXPECT_TRUE(m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::PwmOutput, IO_INDEX,
                                               VirtualPwmOutput::PWM_VALUE_MAX + 1)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::ValueOutOfRange,
                         CustomCanIoType::PwmOutput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetPwmOutputWithLargePeriodDoesNotOverflow)
{
    ON_CALL(m_pwmOutput, period()).WillByDefault(Return(LARGE_PWM_PERIOD));
    EXPECT_CALL(m_pwmOutput, setPulse(LARGE_PWM_PERIOD));
    EXPECT_CALL(m_writeListener, onWritten(Ref(m_virtualPwmOutput)));

    EXPECT_TRUE(m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::PwmOutput, IO_INDEX,
                                               VirtualPwmOutput::PWM_VALUE_MAX)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::PwmOutput, IO_INDEX, VirtualPwmOutput::PWM_VALUE_MAX);
}

TEST_F(CanDispatcherTest, SetAdcInputIsNotSupportedWithoutReading)
{
    EXPECT_CALL(m_adcInput, readMillivolts()).Times(0);

    EXPECT_TRUE(
        m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::AdcInput, IO_INDEX, 42)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::NotSupported,
                         CustomCanIoType::AdcInput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetDigitalInputIsNotSupportedWithoutReading)
{
    EXPECT_CALL(m_digitalInput, read()).Times(0);

    EXPECT_TRUE(
        m_dispatcher.dispatch(setFrame(NODE_ID, CustomCanIoType::DigitalInput, IO_INDEX, 0)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::NotSupported,
                         CustomCanIoType::DigitalInput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetReadbackFailureRespondsBusErrorWithZeroValue)
{
    NiceMock<MockVirtualIo> io;
    ON_CALL(io, ioType()).WillByDefault(Return(IoType::DigitalOutput));
    ON_CALL(io, ioIndex()).WillByDefault(Return(MOCK_IO_INDEX));
    ASSERT_TRUE(m_repository.add(io));
    EXPECT_CALL(io, write(1)).WillOnce(Return(IoWriteResult{}));
    EXPECT_CALL(io, read()).WillOnce(Return(IoReadResult{std::unexpect, IoStatus::ReadError}));
    EXPECT_CALL(m_writeListener, onWritten(Ref(io)));

    EXPECT_TRUE(m_dispatcher.dispatch(
        setFrame(NODE_ID, CustomCanIoType::DigitalOutput, MOCK_IO_INDEX, 1)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::BusError,
                         CustomCanIoType::DigitalOutput, MOCK_IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetUnknownIoIndexRespondsUnknownIoIndex)
{
    EXPECT_CALL(m_digitalOutput, set(_)).Times(0);

    EXPECT_TRUE(m_dispatcher.dispatch(
        setFrame(NODE_ID, CustomCanIoType::DigitalOutput, UNKNOWN_IO_INDEX, 1)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::UnknownIoIndex,
                         CustomCanIoType::DigitalOutput, UNKNOWN_IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetUnknownIoTypeRespondsUnknownIoType)
{
    EXPECT_TRUE(m_dispatcher.dispatch(setFrame(NODE_ID, UNKNOWN_IO_TYPE, IO_INDEX, 1)));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::UnknownIoType,
                         UNKNOWN_IO_TYPE, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetShortPayloadRespondsMalformedPayload)
{
    EXPECT_CALL(m_digitalOutput, set(_)).Times(0);
    CanMessage frame = setFrame(NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX, 1);
    frame.length = 5;

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::MalformedPayload,
                         CustomCanIoType::DigitalOutput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetLongPayloadRespondsMalformedPayload)
{
    EXPECT_CALL(m_digitalOutput, set(_)).Times(0);
    CanMessage frame = setFrame(NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX, 1);
    frame.length = 7;

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::MalformedPayload,
                         CustomCanIoType::DigitalOutput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, SetEmptyPayloadRespondsMalformedPayloadWithInvalidIo)
{
    CanMessage frame = setFrame(NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX, 1);
    frame.length = 0;

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::SetResponse, CustomCanStatus::MalformedPayload,
                         INVALID_IO_TYPE, CUSTOM_CAN_INVALID_IO_FIELD, 0);
}

TEST_F(CanDispatcherTest, GetDigitalOutputReturnsState)
{
    m_digitalState = true;

    EXPECT_TRUE(m_dispatcher.dispatch(getFrame(NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::DigitalOutput, IO_INDEX, 1);
}

TEST_F(CanDispatcherTest, GetDigitalInputReturnsLevel)
{
    ON_CALL(m_digitalInput, read()).WillByDefault(testing::Return(true));

    EXPECT_TRUE(m_dispatcher.dispatch(getFrame(NODE_ID, CustomCanIoType::DigitalInput, IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::DigitalInput, IO_INDEX, 1);
}

TEST_F(CanDispatcherTest, GetPwmOutputReturnsScaledDutyCycle)
{
    m_pwmPulse = 750;

    EXPECT_TRUE(m_dispatcher.dispatch(getFrame(NODE_ID, CustomCanIoType::PwmOutput, IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::PwmOutput, IO_INDEX, 7500);
}

TEST_F(CanDispatcherTest, GetPwmOutputWithLargePeriodDoesNotOverflow)
{
    ON_CALL(m_pwmOutput, period()).WillByDefault(Return(LARGE_PWM_PERIOD));
    m_pwmPulse = 3'000'000'000;

    EXPECT_TRUE(m_dispatcher.dispatch(getFrame(NODE_ID, CustomCanIoType::PwmOutput, IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::PwmOutput, IO_INDEX, 7500);
}

TEST_F(CanDispatcherTest, GetAdcInputReturnsMillivolts)
{
    EXPECT_TRUE(m_dispatcher.dispatch(getFrame(NODE_ID, CustomCanIoType::AdcInput, IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::Ok,
                         CustomCanIoType::AdcInput, IO_INDEX, ADC_MILLIVOLTS);
}

TEST_F(CanDispatcherTest, GetAdcReadFailureRespondsBusError)
{
    ON_CALL(m_adcInput, readMillivolts()).WillByDefault(Return(std::nullopt));

    EXPECT_TRUE(m_dispatcher.dispatch(getFrame(NODE_ID, CustomCanIoType::AdcInput, IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::BusError,
                         CustomCanIoType::AdcInput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, GetUnknownIoIndexRespondsUnknownIoIndex)
{
    EXPECT_TRUE(m_dispatcher.dispatch(
        getFrame(NODE_ID, CustomCanIoType::AdcInput, UNKNOWN_IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::UnknownIoIndex,
                         CustomCanIoType::AdcInput, UNKNOWN_IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, GetUnknownIoTypeRespondsUnknownIoType)
{
    EXPECT_TRUE(m_dispatcher.dispatch(getFrame(NODE_ID, UNKNOWN_IO_TYPE, IO_INDEX)));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::UnknownIoType,
                         UNKNOWN_IO_TYPE, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, GetShortPayloadRespondsMalformedPayload)
{
    CanMessage frame = getFrame(NODE_ID, CustomCanIoType::AdcInput, IO_INDEX);
    frame.length = 1;

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::MalformedPayload,
                         CustomCanIoType::AdcInput, CUSTOM_CAN_INVALID_IO_FIELD, 0);
}

TEST_F(CanDispatcherTest, GetRemoteFrameRespondsMalformedPayloadWithInvalidIo)
{
    CanMessage frame = getFrame(NODE_ID, CustomCanIoType::AdcInput, IO_INDEX);
    frame.remote = true;

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::GetResponse, CustomCanStatus::MalformedPayload,
                         INVALID_IO_TYPE, CUSTOM_CAN_INVALID_IO_FIELD, 0);
}

TEST_F(CanDispatcherTest, ObserveStartRespondsNotSupported)
{
    const CanMessage frame =
        encodeObserveStart(NODE_ID, {{CustomCanIoType::AdcInput, IO_INDEX}, 100, 10});

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::ObserveResponse, CustomCanStatus::NotSupported,
                         CustomCanIoType::AdcInput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, ObserveStopRespondsNotSupported)
{
    const CanMessage frame = encodeObserveStop(NODE_ID, {{CustomCanIoType::AdcInput, IO_INDEX}});

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::ObserveResponse, CustomCanStatus::NotSupported,
                         CustomCanIoType::AdcInput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, ObserveStartShortPayloadRespondsMalformedPayload)
{
    CanMessage frame =
        encodeObserveStart(NODE_ID, {{CustomCanIoType::AdcInput, IO_INDEX}, 100, 10});
    frame.length = 5;

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::ObserveResponse, CustomCanStatus::MalformedPayload,
                         CustomCanIoType::AdcInput, IO_INDEX, 0);
}

TEST_F(CanDispatcherTest, ObserveStopShortPayloadRespondsMalformedPayload)
{
    CanMessage frame = encodeObserveStop(NODE_ID, {{CustomCanIoType::AdcInput, IO_INDEX}});
    frame.length = 1;

    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    expectSingleResponse(CustomCanCommand::ObserveResponse, CustomCanStatus::MalformedPayload,
                         CustomCanIoType::AdcInput, CUSTOM_CAN_INVALID_IO_FIELD, 0);
}

TEST_F(CanDispatcherTest, BroadcastSetWritesWithoutResponse)
{
    EXPECT_CALL(m_digitalOutput, set(true));
    EXPECT_CALL(m_writeListener, onWritten(Ref(m_virtualDigitalOutput)));
    EXPECT_CALL(m_canBus, send(_)).Times(0);

    EXPECT_TRUE(m_dispatcher.dispatch(
        setFrame(CUSTOM_CAN_BROADCAST_NODE, CustomCanIoType::DigitalOutput, IO_INDEX, 1)));
}

TEST_F(CanDispatcherTest, BroadcastSetFailuresSendNoResponse)
{
    EXPECT_CALL(m_canBus, send(_)).Times(0);
    EXPECT_CALL(m_digitalOutput, set(_)).Times(0);
    EXPECT_CALL(m_pwmOutput, setPulse(_)).Times(0);

    CanMessage shortFrame =
        setFrame(CUSTOM_CAN_BROADCAST_NODE, CustomCanIoType::DigitalOutput, IO_INDEX, 1);
    shortFrame.length = 5;

    EXPECT_TRUE(m_dispatcher.dispatch(shortFrame));
    EXPECT_TRUE(
        m_dispatcher.dispatch(setFrame(CUSTOM_CAN_BROADCAST_NODE, UNKNOWN_IO_TYPE, IO_INDEX, 1)));
    EXPECT_TRUE(m_dispatcher.dispatch(setFrame(CUSTOM_CAN_BROADCAST_NODE,
                                               CustomCanIoType::DigitalOutput,
                                               UNKNOWN_IO_INDEX, 1)));
    EXPECT_TRUE(m_dispatcher.dispatch(
        setFrame(CUSTOM_CAN_BROADCAST_NODE, CustomCanIoType::DigitalOutput, IO_INDEX, 2)));
    EXPECT_TRUE(m_dispatcher.dispatch(setFrame(CUSTOM_CAN_BROADCAST_NODE,
                                               CustomCanIoType::PwmOutput, IO_INDEX,
                                               VirtualPwmOutput::PWM_VALUE_MAX + 1)));
    EXPECT_TRUE(m_dispatcher.dispatch(
        setFrame(CUSTOM_CAN_BROADCAST_NODE, CustomCanIoType::AdcInput, IO_INDEX, 1)));
}

TEST_F(CanDispatcherTest, BroadcastGetIsIgnoredWithoutReading)
{
    EXPECT_CALL(m_canBus, send(_)).Times(0);
    EXPECT_CALL(m_adcInput, readMillivolts()).Times(0);

    CanMessage shortFrame = getFrame(CUSTOM_CAN_BROADCAST_NODE, CustomCanIoType::AdcInput, IO_INDEX);
    shortFrame.length = 1;

    EXPECT_TRUE(m_dispatcher.dispatch(
        getFrame(CUSTOM_CAN_BROADCAST_NODE, CustomCanIoType::AdcInput, IO_INDEX)));
    EXPECT_TRUE(m_dispatcher.dispatch(shortFrame));
    EXPECT_TRUE(
        m_dispatcher.dispatch(getFrame(CUSTOM_CAN_BROADCAST_NODE, UNKNOWN_IO_TYPE, IO_INDEX)));
    EXPECT_TRUE(m_dispatcher.dispatch(
        getFrame(CUSTOM_CAN_BROADCAST_NODE, CustomCanIoType::AdcInput, UNKNOWN_IO_INDEX)));
}

TEST_F(CanDispatcherTest, BroadcastObserveStartSendsNoResponse)
{
    EXPECT_CALL(m_canBus, send(_)).Times(0);

    CanMessage frame = encodeObserveStart(CUSTOM_CAN_BROADCAST_NODE,
                                          {{CustomCanIoType::AdcInput, IO_INDEX}, 100, 10});
    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    frame.length = 5;
    EXPECT_TRUE(m_dispatcher.dispatch(frame));
}

TEST_F(CanDispatcherTest, BroadcastObserveStopSendsNoResponse)
{
    EXPECT_CALL(m_canBus, send(_)).Times(0);

    CanMessage frame =
        encodeObserveStop(CUSTOM_CAN_BROADCAST_NODE, {{CustomCanIoType::AdcInput, IO_INDEX}});
    EXPECT_TRUE(m_dispatcher.dispatch(frame));

    frame.length = 1;
    EXPECT_TRUE(m_dispatcher.dispatch(frame));
}

TEST_F(CanDispatcherTest, FrameForOtherNodeIsIgnored)
{
    EXPECT_CALL(m_canBus, send(_)).Times(0);
    EXPECT_CALL(m_digitalOutput, set(_)).Times(0);
    EXPECT_CALL(m_adcInput, readMillivolts()).Times(0);

    EXPECT_FALSE(m_dispatcher.dispatch(
        setFrame(OTHER_NODE_ID, CustomCanIoType::DigitalOutput, IO_INDEX, 1)));
    EXPECT_FALSE(
        m_dispatcher.dispatch(getFrame(OTHER_NODE_ID, CustomCanIoType::AdcInput, IO_INDEX)));
    EXPECT_FALSE(m_dispatcher.dispatch(
        encodeObserveStart(OTHER_NODE_ID, {{CustomCanIoType::AdcInput, IO_INDEX}, 100, 10})));
}

TEST_F(CanDispatcherTest, NonRequestCommandIsNotHandled)
{
    EXPECT_CALL(m_canBus, send(_)).Times(0);

    EXPECT_FALSE(m_dispatcher.dispatch(encodeSetResponse(
        NODE_ID, {CustomCanStatus::Ok, {CustomCanIoType::DigitalOutput, IO_INDEX}, 1})));
    EXPECT_FALSE(m_dispatcher.dispatch(encodeHeartbeat(NODE_ID)));
}

}
