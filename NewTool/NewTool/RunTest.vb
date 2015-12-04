Imports System.Net, System.Net.Sockets, System.Text, System.Threading
Imports System.Runtime.Serialization.Formatters.Binary
Imports System.Runtime.InteropServices

Public Class RunTest

#Region "Required variables"

    '   Frame Requirements
    Private headerFrmType As Byte
    Private payloadLength As UShort
    Private headerFrmID As Byte
    Public gtxTest As New spiTXTestSettings._sPlcSimTxTestParams 'global variable to carry PLC params
    Public rftestParams As New spiTXTestSettings.sRfTxTestParams    'global variable to carry rf params
    Public Const ProductionToolProtocol As Byte = &H8F
    Public Const MAX_FW_VER_LEN As Integer = 16

#End Region
    

    '   <summary>
    '       Enums
    '   </summary>
#Region "Enum Declaration"
    '
    '   States of execution
    '
    Enum states As Byte
        STATE_PREPARE_DUT
        STATE_PREPARE_DUT_CNF
        STATE_PREPARE_REFERENCE
        STATE_PREPARE_REFERENCE_CNF
        STATE_START_TEST
        STATE_START_TEST_DUT
        STATE_START_TEST_REF
        STATE_START_TEST_CNF
        STATE_START_TEST_CNF_DUT
        STATE_START_TEST_CNF_REF
        STATE_STOP_TEST
        STATE_STOP_TEST_CNF
        STATE_DEVICE_RESET
        STATE_DEVICE_RESET_CNF
        STATE_GET_RESULT
        STATE_GET_RESULT_CNF
        STATE_DEVICE_SEARCH
        STATE_DEVICE_UP
        STATE_NONE
    End Enum

    '
    '   Device Type
    '
    Enum ClientType
        DUT
        REF
    End Enum

    '
    '   Device Type
    '
    Enum Response
        FAILURE
        SUCCESS
    End Enum

    '
    '   Test Interface
    '
    Enum TestInterface
        TEST_PLC_ID
        TEST_802_15_5_ID
    End Enum
#End Region

    '   <summary>
    '       Structures
    '   </summary>
#Region "Structure Declerations"
    
    '
    '   Header strucutre
    '
    '<StructLayout(LayoutKind.Explicit, Pack:=1, size:=5)> _
    'Public Structure frmHeader
    '    <FieldOffset(0)> <MarshalAsAttribute(UnmanagedType.U1)> Public protocolID As Byte
    '    <FieldOffset(1)> <MarshalAsAttribute(UnmanagedType.U2)> Public length As UShort
    '    <FieldOffset(3)> <MarshalAsAttribute(UnmanagedType.U1)> Public frmType As Byte
    '    <FieldOffset(4)> <MarshalAsAttribute(UnmanagedType.U1)> Public cmdid As Byte
    'End Structure

    <StructLayout(LayoutKind.Sequential, Pack:=1, size:=5)> _
    Public Structure frmHeader
        Public protocolID As Byte
        Public length As UShort
        Public frmType As Byte
        Public cmdid As Byte
    End Structure

    '
    '   Device UP Event strucutre
    '
    <StructLayout(LayoutKind.Sequential, Size:=22, pack:=1)> _
    Public Structure sDevUpEvent
        Public deviceType As Byte
        Public capabilityInfo As Byte
        <MarshalAs(UnmanagedType.ByValArray, SizeConst:=MAX_FW_VER_LEN)> _
        Public FWVer As Byte()
        Public bMapTests As UInteger
    End Structure

    '
    '   Response strucutre
    '
    <StructLayout(LayoutKind.Sequential, Size:=1, pack:=1)> _
    Public Structure sResponse
        Public rsp As Byte
        'Public calStatus As Byte
    End Structure


    <StructLayout(LayoutKind.Sequential, Size:=1, pack:=1)> _
    Public Structure sProdPrepRfStatusCnf
        Public calStatus As Byte
    End Structure
#End Region
    '
    '   Main form load
    '
    Private Sub RunTest_Load(sender As Object, e As EventArgs) Handles MyBase.Load

    End Sub
    '
    '   Swap stei and dtei
    '
    Private Function swapNWIDsforREF(ByVal x As spiTXTestSettings._sPlcSimTxTestParams) As spiTXTestSettings._sPlcSimTxTestParams
        Dim tmpSTEI As UInteger = 0
        Dim tmpDTEI As UInteger = 0

        tmpSTEI = x.stei
        tmpDTEI = x.dtei

        x.stei = tmpDTEI
        x.dtei = tmpSTEI
        Return x
    End Function

    '
    '   Get interface
    '
    Public Function get_interface(ByVal t As HomeScreen.tests) As TestInterface
        Select Case t
            Case HomeScreen.tests.TEST_ID_PLC_RX
                Return TestInterface.TEST_PLC_ID
                Exit Select
            Case HomeScreen.tests.TEST_ID_PLC_RX_SWEEP
                Return TestInterface.TEST_PLC_ID
                Exit Select
            Case HomeScreen.tests.TEST_ID_PLC_TX
                Return TestInterface.TEST_PLC_ID
                Exit Select
            Case HomeScreen.tests.TEST_ID_PLC_TX_SWEEP
                Return TestInterface.TEST_PLC_ID
                Exit Select

            Case HomeScreen.tests.TEST_ID_RF_RX
                Return TestInterface.TEST_802_15_5_ID
                Exit Select
            Case HomeScreen.tests.TEST_ID_RF_TX
                Return TestInterface.TEST_802_15_5_ID
                Exit Select
            Case HomeScreen.tests.TEST_ID_RF_RX_SWEEP
                Return TestInterface.TEST_802_15_5_ID
                Exit Select
            Case HomeScreen.tests.TEST_ID_RF_TX_SWEEP
                Return TestInterface.TEST_802_15_5_ID
                Exit Select

        End Select
        Return -1
    End Function

    '
    '   fill header for the payload
    '
    Private Sub fill_header(ByRef header As frmHeader, ByVal len As UShort, ByVal cmd As HomeScreen.commandIDs)

        '   fill header 
        header.protocolID = ProductionToolProtocol
        header.frmType = HomeScreen.headerID.headerRequest
        header.length = len
        header.cmdid = cmd

    End Sub

    '   <summary>
    '       Prepare payload to send to device
    '   </summary>
    Private Function getPayload(ByVal state As states, ByVal test As HomeScreen.tests) As Byte()

        Dim txMsg() = New Byte() {}
        Dim header As New frmHeader
        Dim payloadLen As UShort
        Dim structByte() = New Byte() {}

        Select Case state

            Case states.STATE_PREPARE_DUT

                If get_interface(test) = TestInterface.TEST_PLC_ID Then
                    structByte = StructToByte(gtxTest)
                ElseIf get_interface(test) = TestInterface.TEST_802_15_5_ID Then
                    structByte = StructToByte(rftestParams)
                End If
                payloadLen = structByte.Length
                'payloadLen += 1 ' for test
                payloadLen += 2 ' for test and interface

                '   fill header 
                fill_header(header, payloadLen, HomeScreen.commandIDs.TOOL_CMD_PREPARE_DUT)

                Dim headerByte As Byte() = StructToByte(header)
                '   fill payload
                Array.Resize(txMsg, headerByte.Length)
                Array.Copy(headerByte, 0, txMsg, 0, headerByte.Length)

                '   Add interface
                Add(txMsg, CType([Enum].Parse(GetType(TestInterface), get_interface(test)), TestInterface))
                '   fill in the test
                Add(txMsg, test)

                '   fill the test parameters
                For Each b As Byte In structByte
                    Add(txMsg, b)
                Next

                Exit Select

            Case states.STATE_PREPARE_REFERENCE

                Dim gtxTest_forREF As spiTXTestSettings._sPlcSimTxTestParams = Nothing

                If get_interface(test) = TestInterface.TEST_PLC_ID Then
                    gtxTest_forREF = swapNWIDsforREF(gtxTest)
                    structByte = StructToByte(gtxTest_forREF)
                ElseIf get_interface(test) = TestInterface.TEST_802_15_5_ID Then
                    structByte = StructToByte(rftestParams)
                End If

                payloadLen = structByte.Length
                'payloadLen += 1 ' for test
                payloadLen += 2 ' for test and interface

                '   fill header 
                fill_header(header, payloadLen, HomeScreen.commandIDs.TOOL_CMD_PREPARE_REFERENCE)

                Dim headerByte As Byte() = StructToByte(header)
                '   fill payload
                Array.Resize(txMsg, headerByte.Length)
                Array.Copy(headerByte, 0, txMsg, 0, headerByte.Length)

                '   Add interface
                Add(txMsg, CType([Enum].Parse(GetType(TestInterface), get_interface(test)), TestInterface))
                '   fill in the test
                Add(txMsg, test)

                '   fill the test parameters
                For Each b As Byte In structByte
                    Add(txMsg, b)
                Next
                Exit Select

            Case states.STATE_START_TEST
                'payloadLen = 1
                payloadLen = 2   'test id and interface

                '   fill header 
                fill_header(header, payloadLen, HomeScreen.commandIDs.TOOL_CMD_START_TEST)

                Dim headerByte As Byte() = StructToByte(header)
                '   fill payload
                Array.Resize(txMsg, headerByte.Length)
                Array.Copy(headerByte, 0, txMsg, 0, headerByte.Length)

                '   Add interface
                Add(txMsg, CType([Enum].Parse(GetType(TestInterface), get_interface(test)), TestInterface))
                '   fill payload
                Add(txMsg, test)
                Exit Select

            Case states.STATE_STOP_TEST
                payloadLen = 1

                '   fill header 
                fill_header(header, payloadLen, HomeScreen.commandIDs.TOOL_CMD_STOP_TEST)

                Dim headerByte As Byte() = StructToByte(header)
                '   fill payload
                Array.Resize(txMsg, headerByte.Length)
                Array.Copy(headerByte, 0, txMsg, 0, headerByte.Length)
                '   Add interface
                Add(txMsg, CType([Enum].Parse(GetType(TestInterface), get_interface(test)), TestInterface))

                Exit Select

            Case states.STATE_DEVICE_RESET
                payloadLen = 0

                '   fill header 
                fill_header(header, payloadLen, HomeScreen.commandIDs.TOOL_CMD_DEVICE_RESET)

                Dim headerByte As Byte() = StructToByte(header)
                '   fill payload
                Array.Resize(txMsg, headerByte.Length)
                Array.Copy(headerByte, 0, txMsg, 0, headerByte.Length)

                Exit Select

            Case states.STATE_DEVICE_SEARCH
                payloadLen = 0

                '   fill header 
                fill_header(header, payloadLen, HomeScreen.commandIDs.TOOL_CMD_DEVICE_SEARCH)

                Dim headerByte As Byte() = StructToByte(header)
                '   fill payload
                Array.Resize(txMsg, headerByte.Length)
                Array.Copy(headerByte, 0, txMsg, 0, headerByte.Length)

                Exit Select

            Case states.STATE_GET_RESULT
                payloadLen = 1

                '   fill header 
                fill_header(header, payloadLen, HomeScreen.commandIDs.TOOL_CMD_GET_RESULT)

                Dim headerByte As Byte() = StructToByte(header)
                '   fill payload
                Array.Resize(txMsg, headerByte.Length)
                Array.Copy(headerByte, 0, txMsg, 0, headerByte.Length)
                '   Add interface
                Add(txMsg, CType([Enum].Parse(GetType(TestInterface), get_interface(test)), TestInterface))

                Exit Select

        End Select
        '   Return transmit payload
        Return txMsg
    End Function

    '   <summary>
    '       Async send 
    '   </summary>
#Region "Send Thread"
    '
    '   start send thread
    '
    Public Sub beginSend(ByVal state As states, mClient As ConnectedClient, ByVal t As HomeScreen.tests)

        Dim txMsg() = New Byte() {}
        txMsg = getPayload(state, t)
        
        If txMsg IsNot Nothing Then
            mClient.SendMessage(txMsg)
        End If

    End Sub

#End Region

    '   <summary>
    '       General purpose utility functions needed 
    '   </summary>
#Region "Utility functions"

    '   <summary>
    '      Convert structure to Byte
    '   </summary>
    Public Function StructToByte(ByVal Struct As Object) As Byte()

        Dim iStructSize As Integer = Marshal.SizeOf(Struct)
        Dim buffer As IntPtr = Marshal.AllocHGlobal(iStructSize)

        Marshal.StructureToPtr(Struct, buffer, False)
        Dim btData(iStructSize - 1) As Byte

        Marshal.Copy(buffer, btData, 0, iStructSize)
        Marshal.FreeHGlobal(buffer)

        Return btData

    End Function

    '   <summary>
    '      Serialize data
    '   </summary>
    Shared Function Serialize(ByVal data As Object) As Byte()
        If TypeOf data Is Byte() Then Return data
        Using M As New IO.MemoryStream : Dim F As New BinaryFormatter : F.Serialize(M, data) : Return M.ToArray() : End Using
    End Function

    '   <summary>
    '      Deserialize data
    '   </summary>
    Shared Function Deserialize(Of T)(ByVal data As Byte()) As T
        Using M As New IO.MemoryStream(data, False) : Return CType((New BinaryFormatter).Deserialize(M), T) : End Using
    End Function

    '   <summary>
    '      Convert Structure to string
    '   </summary>
    Public Function StructToString(obj As Object) As String

        Return String.Join("", obj.GetType().GetFields().Select(Function(field) field.GetValue(obj)))

    End Function

    '   <summary>
    '      Add element to the array
    '   </summary>
    Public Sub Add(Of T)(ByRef arr As T(), item As T)
        If arr Is Nothing Then
            arr(0) = item
        Else
            Array.Resize(arr, arr.Length + 1)
            arr(arr.Length - 1) = item
        End If
    End Sub

#End Region

    '
    '   check if multiple tests are selected
    '
    Private Function isMultipleTestSelected(ByVal SelectedTests As Byte()) As Boolean
        Dim count As Integer = 0
        If HomeScreen.chkbxPLCTX.Checked Then
            count += 1
            Add(SelectedTests, HomeScreen.tests.TEST_ID_PLC_TX)
        End If
        If HomeScreen.chkbx_RFTX.Checked Then
            count += 1
            Add(SelectedTests, HomeScreen.tests.TEST_ID_PLC_RX)
        End If
        If HomeScreen.chkbxPLCRX.Checked Then
            count += 1
            Add(SelectedTests, HomeScreen.tests.TEST_ID_RF_TX)
        End If

        If count > 1 Then
            Return True
        Else
            Return False
        End If
        Return Nothing
    End Function
    '
    '   Dispose form
    '
    Private Sub Button6_Click(sender As Object, e As EventArgs) Handles Button6.Click
        Me.Dispose()
    End Sub
End Class