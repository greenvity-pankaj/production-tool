Imports System, System.Net, System.Net.Sockets, System.Windows.Forms
Imports System.Text, System.IO, System.Threading
Imports System.Collections, System.Collections.Generic, System.ComponentModel, System.Management
Imports System.Runtime.InteropServices
Imports System.Net.NetworkInformation
Imports System.Resources
Imports System.Globalization

Public Class HomeScreen

    '
    '   Bitmap for tests
    '
#Region "Bitmap for tests"
    '
    '   32  |   31  |   30  |   29  |   28  |   27  |   26  |   25  |
    '
    '       |       |       |       |       |       |       |       |
    '
    '   24  |   23  |   22  |   21  |   20  |   19  |   18  |   17  |
    '
    '       |       |       |       |       |       |       |       |
    '
    '   16  |   15  |   14  |   13  |   12  |   11  |   10  |   09  |
    '
    '       |       |       |       |       |       |       |       |
    '
    '   08  |   07  |   06  |   05  |   04  |   03  |   02  |   01  |
    '
    '       |       |       |       |PLC_RX_SWEEP|PLC_TX_SWEEP| PLC RX| PLC TX|

    Enum bitmap
        PLC_TX
        PLC_RX
        PLC_TX_SWEEP
        PLC_RX_SWEEP
        RF_TX = 32
        RF_RX = 33
    End Enum

    Enum capability
        PLC = 1
        RF = 2
        RF_PLC = 3
    End Enum

    Private Const BOARD_TYPE_GV7011_MOD = "3"
    Private Const BOARD_TYPE_GV7011_LED = "6"

#End Region

    '
    '   Structure Declaration
    '
#Region "Structure Declaration"
    '
    '   To maintain the execution status
    '
    Public Structure execState
        Public test As tests
        Public state As RunTest.states
    End Structure

    '
    '   Metadata to log the results
    '
    Public Structure metadata
        Public newTestRun As Boolean
        Public testStatus As Boolean
        Public name As String
        Public DUT As String
        Public REF As String
        Public runCount As UInteger
        Public threshold As UInteger
        Public startTime As String
        Public stopTime As String
        Public duration As String
        Public filePath As String
        Public serialNum As String
        Public lotnumber As String
        Public testPramas As TestSettings._sPlcSimTxTestParams
        Public rsp As shpgpStats._plcTxTestResults_t
        Public rftestPramas As TestSettings.sRfTxTestParams
        Public rf_rsp As shpgpStats._sRfStats
        Public intf As RunTest.TestInterface
    End Structure
    '
    '   list of individual tests and variations and results for a board
    '
    Public Structure variations
        Public name As String
        Public result As Boolean
    End Structure
    '
    '   Summary of the board > tests done and individual results
    '
    Public Structure summary
        Public filepath As String
        Public xlfilepath As String
        Public serialNum As String
        Public lotnumber As String
        Public MAC As String
        Public finalResult As String
        Public tests As List(Of variations)
    End Structure

#End Region

    '   <summary>
    '       Declaration of variables used in the program
    '   </summary>
#Region "Required Variables"

    '   Boolean Variables
    Private DUT_UP As New Boolean
    Private REF_UP As New Boolean
    Private lineSent As New Boolean
    Private devicesUP As New Boolean
    Private testRunning As New Boolean
    Private setPowerMode As New Boolean
    Public Shared userPLCTXPramasSet As New Boolean
    Public Shared userPLCRXPramasSet As New Boolean
    Private sweepTestRunning As New Boolean

    '   Delegates
    Private Delegate Sub _initUIDelegate()
    Private Delegate Sub _addtoLVClient(s As String)
    Private Delegate Function getSelectedLVIItemDelegate()
    Private Delegate Sub _AddClient(ByVal client As Socket, ByVal type As RunTest.ClientType)

    '   Directory
    Private clDirectory = New Dictionary(Of String, ConnectedClient)

    '   Enum Variables
    Public Shared testParamsFor As tests
    Private DUTCapbility As New capability
    Private REFCapbility As New capability

    '   Duration Calculations
    Private stop_time As DateTime = Nothing
    Private start_time As DateTime = Nothing
    Private elapsed_time As TimeSpan = Nothing

    '   Integers Variables
    Public Shared plcTestThreshold As Decimal = 10
    Public Shared rfTestThreshold As Decimal = 10
    Private minVal As Decimal = 0
    Private maxVal As Decimal = 130
    Private port As Integer = 8080 '54321
    Private swpCount As New UInteger
    Private runCount As New UInteger
    Private serverThreadCount As Integer = 0
    Private pendingConnections As Integer = 5
    Private RF_FRM_NUM As Integer = 100
    Public Shared RF_CHANNEL As Byte = &HF
    Public Shared gMACcounter = New ULong
    Public Shared gMACAddress = New ULong
    Public Shared serialNumber As String = ""
    '   Lists
    Private sockets As List(Of Socket)
    Private clients As New List(Of ConnectedClient)
    Private qExecStatus As New List(Of tests)
    Private defTestQ As New List(Of tests)
    Private FrmLenArr As New List(Of UInteger)(New UInteger() {100, 500})
    'Private RFChannelList As New List(Of Byte)(New Byte() {&HF, &H14, &H1A})
#If TEST_CHANNEL = "15" Then
    Private RFChannelList As New List(Of Byte)(New Byte() {15}) ' List of channels in Sweep Test Queue
#ElseIf TEST_CHANNEL = "16" Then
    Private RFChannelList As New List(Of Byte)(New Byte() {16}) ' List of channels in Sweep Test Queue
#ElseIf TEST_CHANNEL = "25" Then
    Private RFChannelList As New List(Of Byte)(New Byte() {25}) ' List of channels in Sweep Test Queue
#ElseIf TEST_CHANNEL = "26" Then
    Private RFChannelList As New List(Of Byte)(New Byte() {26}) ' List of channels in Sweep Test Queue
#End If

    ' Objects   >> used for UI object control
    Private objForUI = New Object()
    Public objForMac = New Object
    '   Queue
    Private sweepParamList As New Queue(Of TestSettings._sPlcSimTxTestParams)
    Private RFChannelParamList As New Queue(Of TestSettings.sRfTxTestParams)

    '   Listener
    Private listener As System.Net.Sockets.TcpListener

    '   String Variables
    Private st As String
    Private filereader As String
    Public Shared rootFilePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "Production Tool Logs")

    '   Structure Variables
    Public Shared m As metadata
    Public Shared s As New summary
    Public Shared status As New execState
    Public Shared plcRXparams As New TestSettings._sPlcSimTxTestParams
    Public Shared plcTXparams As New TestSettings._sPlcSimTxTestParams
    Public Shared gtxTest As New TestSettings._sPlcSimTxTestParams  'global variable to carry PLC test parameter
    Public Shared rfgtxTest As New TestSettings.sRfTxTestParams  'global variable to carry RF test parameter

    '   Threads
    Private listenThread As System.Threading.Thread

#End Region

    '   <summary>
    '       Declaration of enums used in the program
    '   </summary>
#Region "Enum Declerations"
    '
    '   Command IDs
    '
    Enum commandIDs As Byte
        TOOL_CMD_PREPARE_DUT = &H0
        TOOL_CMD_PREPARE_DUT_CNF = &H80
        TOOL_CMD_PREPARE_REFERENCE = &H1
        TOOL_CMD_PREPARE_REFERENCE_CNF = &H81
        TOOL_CMD_START_TEST = &H2
        TOOL_CMD_START_TEST_CNF = &H82
        TOOL_CMD_STOP_TEST = &H3
        TOOL_CMD_STOP_TEST_CNF = &H83
        TOOL_CMD_DEVICE_RESET = &H4
        TOOL_CMD_DEVICE_RESET_CNF = &H84
        TOOL_CMD_GET_RESULT = &H5
        TOOL_CMD_GET_RESULT_CNF = &H85
        TOOL_CMD_DEVICE_SEARCH = &H6
        TOOL_CMD_DEVICE_FLASH_PARAM = &H7
        TOOL_CMD_DEVICE_FLASH_PARAM_CNF = &H87
        TOOL_CMD_DEVICE_SPI_DISCONNECT = &H8
    End Enum
    '
    '   List of Events
    '
    Enum testEvents As Byte
        DEVICE_UP
        TEST_DONE
    End Enum
    '
    '   List of tests
    '
    Enum tests As Byte
        TEST_ID_PLC_TX
        TEST_ID_PLC_RX
        TEST_ID_PLC_TX_SWEEP
        TEST_ID_PLC_RX_SWEEP
        TEST_ID_RF_TX = 32
        TEST_ID_RF_RX = 33
        TEST_ID_RF_TX_SWEEP
        TEST_ID_RF_RX_SWEEP
        TEST_ID_FLASH_CONFIG
        TEST_NONE
    End Enum
    '
    '   Header Event/Command IDs
    '
    Enum headerID
        headerEvent
        headerRequest
        headerResponse
    End Enum
    '
    '   Response 
    '
    Enum response As Byte
        SUCCESS
        INVALID
        FAILED = &HFF
    End Enum
    '
    ' RF Calibration Response
    '
    Enum RFCalibStatus As Byte
        RF_CALIBRATION_FAILED
        RF_CALIBRATED
        RF_NOT_CALIBRATED = &HFF
    End Enum

#End Region

    '
    '   Main form dispose event 
    '   Terminates all socket connections and disposes TCP Client
    '

    'Declare Function SetSysColors Lib "user32" (ByVal nChanges As Long, lpSysColor As Long, lpColorValues As Long) As Long
    'Public Const COLOR_WINDOWFRAME As Integer = 6
    '<DllImport("User32")>
    'Private Shared Function SetSysColors(ByVal one As Integer, ByRef element As Integer, ByRef color As Integer) As Boolean
    'End Function
    'Private Function Color2COLORREF(ByVal color As Color) As Integer
    '    Return color.R Or (color.G << 8) Or (color.B << &H10)
    'End Function

    Private Sub HomeScreen_Disposed(sender As Object, e As EventArgs) Handles Me.Disposed

    End Sub

    '
    '   Main form load
    '
    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load

        ' To allow cross thread access without delegates, however in future developments, 
        ' this shoud be set to true and delegates should be used 
        Control.CheckForIllegalCrossThreadCalls = False
        panel_disconnect_info.Visible = False
        lblResult.Text = ""
        ' To read the xml settings file for the tool
        Dim s As New readConfig

        Me.Text = "Greenvity Production Tool v" & Me.ProductVersion
        lblSessionLocation.Text = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "Production Tool Logs")

#If TEST_CHANNEL = "15" Then
        lbl_rf_ch_no.Text = 15
#ElseIf TEST_CHANNEL = "16" Then
    lbl_rf_ch_no.Text = 16
#ElseIf TEST_CHANNEL = "25" Then
    lbl_rf_ch_no.Text = 25
#ElseIf TEST_CHANNEL = "26" Then
    lbl_rf_ch_no.Text = 26
#End If

        s.readXML()
        'gMACAddress = gMACcounter
        initUI()
        startServer()

        lbl_flashDone.Text = ""
        'lbl_flashDone.ForeColor = Color.DarkRed
    End Sub

    '
    '   initialize main UI screen (Invoke by delegate)
    '
    Private Sub initUI()
        If InvokeRequired Then
            Me.Invoke(New _initUIDelegate(AddressOf initUI))
        Else
            btnRunTest.Enabled = False
            btn_ShowResults.Enabled = False
            btnStopTest.Enabled = False
            btn_ResetDevices.Enabled = False

            'PLC RX
            chkbxPLCRX.Enabled = False
            btnPLCRXSettings.Enabled = False
            'PLC TX
            chkbxPLCTX.Enabled = False
            btnPLCTXSettings.Enabled = False
            'RF TX
            chkbxRFTX.Enabled = False
            btnRFTXSetting.Enabled = False
            chkbx_RFTXSweep.Enabled = False
            btn_RFTXSweepSetting.Enabled = False
            'RF RX
            chkbxRFRX.Enabled = False
            btnRFRXSetting.Enabled = False
            chkbx_RFRXSweep.Enabled = False
            btn_RFRXSweepSetting.Enabled = False

            '   Set bar thresholds
            bar.Maximum = maxVal
            bar.Minimum = minVal
            bar.Step = 10

            setStatusvarNull()
        End If
    End Sub

    '
    '   Start Server
    '
    Private Sub startServer()

        Try
            listener = New System.Net.Sockets.TcpListener(System.Net.IPAddress.Any, port)
            listener.Start()

            listenThread = New System.Threading.Thread(AddressOf doListen)
            listenThread.IsBackground = True
            listenThread.Start()
        Catch ex As SocketException
            MessageBox.Show(ex.ToString)
        End Try

    End Sub

    '
    '   Delegate for adding item to lvClient
    '
    Private Sub addtoLVClient(ByVal s As String)
        lvClients.Items.Add(s)
    End Sub
    '
    '   Listen to incoming connections
    '
    Private Sub doListen()

        Do
            Dim incomingClient As New System.Net.Sockets.TcpClient
            incomingClient = listener.AcceptTcpClient
            Dim connClient As New ConnectedClient(incomingClient)
            '   Event for data received
            AddHandler connClient.dataReceived, AddressOf Me.messageReceived
            clients.Add(connClient)
            If lvClients.InvokeRequired Then
                Dim d As New _addtoLVClient(AddressOf addtoLVClient)
                Me.Invoke(d, New Object() {incomingClient.Client.RemoteEndPoint.ToString.Split(":"c).First})
            Else
                addtoLVClient(incomingClient.Client.RemoteEndPoint.ToString.Split(":"c).First)
            End If
            Threading.Thread.Sleep(5)
        Loop

    End Sub

    '
    '   Add client (invoke by Delegate)
    '
    Public Sub AddClient(ByVal client As Socket, ByVal type As RunTest.ClientType)

        Dim clIP As String() = client.RemoteEndPoint.ToString.Split(":"c)

        Dim ip As String = clIP.First
        lvClients.SelectedIndex = lvClients.FindString(ip)
        If lvClients.SelectedIndex <> -1 Then
            lvClients.Items.RemoveAt(lvClients.SelectedIndex)
        End If

        Dim lvi As New ListViewItem(clIP.First & " " & type.ToString)
        lvi.Text = clIP.First & " " & type.ToString
        lvi.Tag = client
        lvClients.Items.Add(lvi.Text)

    End Sub

    '
    '   Remove client
    '   Unused
    '
    Private Sub removeClient(ByVal client As ConnectedClient)

        If clients.Contains(client) Then
            clients.Remove(client)
            dumpMsg("Client removed !!")
        End If

    End Sub

    '
    '   Scan devices
    '
    Private Sub scan()
        Try
            If clients.Count > 0 Then

                btnRunTest.Enabled = False
                SyncLock objForUI
                    txtbxDummy.Clear()
                End SyncLock
                DUT_UP = False
                REF_UP = False
                devicesUP = False
                Try
                    If lvClients.SelectedItems.Count > 0 Then
                        lvClients.ClearSelected()
                    End If
                    If lvClients.Items.Count > 0 Then
                        lvClients.Items.Clear()
                    End If

                Catch ex1 As Exception
                    MessageBox.Show(ex1.ToString)
                    lvClients = New ListBox
                End Try

                For Each conClient As ConnectedClient In clients
                    If conClient.mClient.Connected Then
                        RunTest.beginSend(RunTest.states.STATE_DEVICE_SEARCH, conClient, Nothing)
                    Else
                        'conClient.mClient.Client.Close()
                        'removeClient(conClient)
                    End If
                Next
            Else
                MessageBox.Show("No clients Detected !")
                MessageBox.Show("Make sure ARM9 clients are up !")
                Exit Sub
            End If
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

    '
    '   Scan Devices Button Event
    '
    Private Sub btn_ScanDevices_Click_1(sender As Object, e As EventArgs) Handles btn_ScanDevices.Click
        scan()
    End Sub

    '
    '   check if both devices are up
    '
    Private Function isDeviceUP(ByVal sender As ConnectedClient, ByVal deviceUPEvent As RunTest.sDevUpEvent) As Boolean

        If deviceUPEvent.deviceType = RunTest.ClientType.REF Then
            sender.mDevType = deviceUPEvent.deviceType
            clDirectory(sender.mClient.Client.RemoteEndPoint.ToString.Split(":"c).First) = sender
            If lvClients.InvokeRequired Then
                Dim d As New _AddClient(AddressOf AddClient)
                Me.Invoke(d, New Object() {sender.mClient.Client, RunTest.ClientType.REF})
            Else
                AddClient(sender.mClient.Client, RunTest.ClientType.REF)
            End If
            REF_UP = True
            REFCapbility = deviceUPEvent.capabilityInfo
        End If

        If deviceUPEvent.deviceType = RunTest.ClientType.DUT Then
            sender.mDevType = deviceUPEvent.deviceType
            clDirectory(sender.mClient.Client.RemoteEndPoint.ToString.Split(":"c).First) = sender
            If lvClients.InvokeRequired Then
                Dim d As New _AddClient(AddressOf AddClient)
                Me.Invoke(d, New Object() {sender.mClient.Client, RunTest.ClientType.DUT})
            Else
                AddClient(sender.mClient.Client, RunTest.ClientType.DUT)
            End If
            DUT_UP = True
            DUTCapbility = deviceUPEvent.capabilityInfo
        End If

        If DUT_UP = True And REF_UP = True Then
            devicesUP = True
            enableTests2(DUTCapbility)          ' Enable tests on DUT capability
            txtbxDummy.Clear()
            dumpMsg("Tests are configured ! ")
            dumpMsg("You can start the tests ! ")
            dumpMsg(vbNewLine)
            btnRunTest.Enabled = True
            Return devicesUP
        End If

        Return False

    End Function

    '
    '   get the selected client
    '
    Public Function getSelectedClientList() As List(Of ConnectedClient)

        Dim list = New List(Of ConnectedClient)
        Dim cl As ConnectedClient

        Try
            If lvClients.Items.Count <= 1 Then
                scan()
            Else
                For Each s As String In lvClients.Items
                    cl = clDirectory(s.Split(" "c).First)
                    If cl.mClient.Connected Then
                        list.Add(cl)
                    End If
                Next
            End If
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
            scan()
        End Try

        Return list
    End Function

    '
    '   Enable tests per interface
    '
    Private Sub enableTests2(ByRef c As capability)
        Select Case c
            Case capability.PLC
                ' PLC tests
                setDefaultPLCPramas()
                ' enable PLC TX & RX
                chkbxPLCTX.Enabled = True
                btnPLCTXSettings.Enabled = True
                chkbxPLCRX.Enabled = True
                btnPLCRXSettings.Enabled = True

                ' PLC TX Sweep
                chkbx_PLCTXSweep.Enabled = True
                chkbx_PLCTXSweep.Checked = True
                btn_PLCTXSweepSettings.Enabled = True
                ' PLC RX Sweep
                chkbx_PLCRXSweep.Enabled = True
                chkbx_PLCRXSweep.Checked = True
                btn_PLCRXSweepSettings.Enabled = True
                ' Enable run tests button
                btnRunTest.Enabled = True
                Exit Select

            Case capability.RF_PLC
                ' PLC tests
                setDefaultPLCPramas()
                ' enable PLC TX & RX
                chkbxPLCTX.Enabled = True
                btnPLCTXSettings.Enabled = True
                chkbxPLCRX.Enabled = True
                btnPLCRXSettings.Enabled = True
#If LED_BOARD_TEST = "YES" Then
                chkbxPLCTX.Checked = True
                chkbxPLCRX.Checked = True
#Else

#End If
                ' PLC TX Sweep
                chkbx_PLCTXSweep.Enabled = True
                btn_PLCTXSweepSettings.Enabled = True
                ' PLC RX Sweep
                chkbx_PLCRXSweep.Enabled = True
                btn_PLCRXSweepSettings.Enabled = True
#If LED_BOARD_TEST = "YES" Then

#ElseIf LED_BOARD_TEST = "NO" Then
                chkbx_PLCTXSweep.Checked = True
                chkbx_PLCRXSweep.Checked = True
#Else

#End If
                ' RF tests
                setDefaultRFPramas()
                ' enable RF TX & RX
                chkbxRFTX.Enabled = True
                btnRFTXSetting.Enabled = True
                chkbxRFRX.Enabled = True
                btnRFRXSetting.Enabled = True

                ' RF TX Sweep Test
                chkbx_RFTXSweep.Enabled = True
                chkbx_RFTXSweep.Checked = True
                ' RF RX Sweep Test
                chkbx_RFRXSweep.Enabled = True
                chkbx_RFRXSweep.Checked = True

                ' Enable run tests button
                btnRunTest.Enabled = True
                Exit Select

            Case capability.RF
                ' RF tests
                setDefaultRFPramas()
                ' enable RF TX & RX
                chkbxRFTX.Enabled = True
                btnRFTXSetting.Enabled = True
                chkbxRFRX.Enabled = True
                btnRFRXSetting.Enabled = True

                ' RF TX Sweep Test
                chkbx_RFTXSweep.Enabled = True
                chkbx_RFTXSweep.Checked = True
                ' RF RX Sweep Test
                chkbx_RFRXSweep.Enabled = True
                chkbx_RFRXSweep.Checked = True

                ' Enable run tests button
                btnRunTest.Enabled = True
                Exit Select
        End Select
    End Sub

    '
    '   Enable UI tests as per bitmap
    '
    Private Sub enableTests(ByVal bMap As UInteger)

        Dim bitNum As Integer = 31
        Dim test As Byte
        Dim s As String = Convert.ToString(bMap, 2).PadLeft(32, "0")

        For Each c As Char In s
            If c = "1" Then
                test = CType(bitNum, bitmap)
                Select Case test
                    Case bitmap.PLC_TX

                    Case bitmap.PLC_RX

                    Case bitmap.PLC_TX_SWEEP

                    Case bitmap.PLC_RX_SWEEP
                        '' PLC TX
                        'chkbxPLCTX.Enabled = True
                        'btnSettingsPLCTX.Enabled = True
                        '' PLC RX
                        'chkbxPLCRX.Enabled = True
                        'btnSettingsPLCRX.Enabled = True

                        setDefaultPLCPramas()

                        ' PLC TX Sweep
                        chkbx_PLCTXSweep.Enabled = True
                        chkbx_PLCTXSweep.Checked = True
                        btn_PLCTXSweepSettings.Enabled = True
                        ' PLC RX Sweep
                        chkbx_PLCRXSweep.Enabled = True
                        chkbx_PLCRXSweep.Checked = True
                        btn_PLCRXSweepSettings.Enabled = True
                        ' Enable run tests button
                        btnRunTest.Enabled = True
                        Exit Select

                    Case bitmap.RF_TX

                        setDefaultRFPramas()

                        chkbxRFTX.Enabled = True
                        chkbx_PLCTXSweep.Checked = True
                        chkbx_PLCRXSweep.Checked = True
                        btn_PLCRXSweepSettings.Enabled = True
                        btn_PLCTXSweepSettings.Enabled = True
                        ' Enable run tests button
                        btnRunTest.Enabled = True
                        Exit Select
                End Select
                bitNum -= 1
            Else
                bitNum -= 1
            End If
        Next
    End Sub

    '
    '   Issue command stop test
    '
    Private Sub issueCmd_StopTest(ByVal t As tests)
        ' for PLC TX and PLC TX Sweep
        If t = tests.TEST_ID_PLC_TX Or t = tests.TEST_ID_PLC_TX_SWEEP Then
            status.test = tests.TEST_ID_PLC_TX
        End If
        ' for PLC RX and PLC RX Sweep
        If t = tests.TEST_ID_PLC_RX Or t = tests.TEST_ID_PLC_RX_SWEEP Then
            status.test = tests.TEST_ID_PLC_RX
        End If
        ' for RF TX
        If t = tests.TEST_ID_RF_TX Or t = tests.TEST_ID_RF_TX_SWEEP Then
            status.test = tests.TEST_ID_RF_TX
        ElseIf t = tests.TEST_ID_RF_RX Or t = tests.TEST_ID_RF_RX_SWEEP Then
            status.test = tests.TEST_ID_RF_RX
        End If

        status.state = RunTest.states.STATE_STOP_TEST
    End Sub

    '
    '   Stop Test
    '
    Private Sub btnStopTest_Click(sender As Object, e As EventArgs) Handles btnStopTest.Click

        issueCmd_StopTest(status.test)
        executeTests(status)
        clearBar()

    End Sub

    '
    '   Set the status variable of state machine to neutral state
    '
    Private Sub setStatusvarNull()
        status.test = tests.TEST_NONE
        status.state = RunTest.states.STATE_NONE
    End Sub

    '
    '   If incorrect device up event is received
    '
    Private Sub incorrectDevUpEvent()
        MsgBox("Device Up Event received while tests were running !", MsgBoxStyle.Critical)
        lblSessionName.Text = "Resetting ..."

        dumpMsg("Device Up Event received while tests were running !")
        dumpMsg("Resetting ...")

        setStatusvarNull()
        swpCount = 0
        runCount = 0

        start_time = Nothing
        stop_time = Nothing
        elapsed_time = Nothing
        testRunning = False
        sweepTestRunning = False

        ' clear test parameter queues
        qExecStatus.Clear()
        sweepParamList.Clear()
        RFChannelParamList.Clear()

        enableUI()
        clearBar()

        chkbx_PLCRXSweep.Checked = False
        chkbx_PLCTXSweep.Checked = False
        chkbx_RFRXSweep.Checked = False
        chkbx_RFTXSweep.Checked = False
        chkbxPLCRX.Checked = False
        chkbxPLCTX.Checked = False
        chkbxRFRX.Checked = False
        chkbxRFTX.Checked = False

        btn_ShowResults.Enabled = False
        btnRunTest.Enabled = False

        Threading.Thread.Sleep(1000)
        lblSessionName.Text = String.Empty
    End Sub

    '   <summary>
    '       State machine to handle the received message
    '   </summary>
#Region "Received Message Handler"
    '
    '   message received
    '
    Public Sub messageReceived(ByVal sender As ConnectedClient, ByVal packet As Byte())

        Dim headerArray As New List(Of Byte)
        Dim respArray(Marshal.SizeOf(GetType(RunTest.sResponse))) As Byte

        '   If the protocol is a match
        If (RunTest.ProductionToolProtocol = packet(0)) Then

            For Each b As Byte In packet
                headerArray.Add(b)
                If headerArray.Count = Marshal.SizeOf(GetType(RunTest.frmHeader)) Then
                    Exit For
                End If
            Next

            '   Typecast Header
            Dim packetHeader As New RunTest.frmHeader
            packetHeader = CType(ByteToStruct(headerArray.ToArray, GetType(RunTest.frmHeader)), RunTest.frmHeader)

            Select Case packetHeader.frmType
                '
                '   State machine for Event
                '
                Case headerID.headerEvent

                    Select Case packetHeader.cmdid

                        Case testEvents.DEVICE_UP
                            '
                            '   Device UP Event
                            '
                            ' if the device up event is received while the test is running
                            ' then ignore all the other states and reset everything
                            If testRunning Or sweepTestRunning Then
                                incorrectDevUpEvent()
                                Exit Sub
                            End If

                            Dim upEventArray As New List(Of Byte)
                            For Each b As Byte In packet.ToList.GetRange(Marshal.SizeOf(GetType(RunTest.frmHeader)),
                                                                         Marshal.SizeOf(GetType(RunTest.sDevUpEvent)))
                                upEventArray.Add(b)
                                If upEventArray.Count = Marshal.SizeOf(GetType(RunTest.sDevUpEvent)) Then
                                    Exit For
                                End If
                            Next

                            Dim deviceUPEvent As New RunTest.sDevUpEvent
                            deviceUPEvent = CType(ByteToStruct(upEventArray.ToArray,
                                                               GetType(RunTest.sDevUpEvent)), RunTest.sDevUpEvent)
                            SyncLock objForUI
                                isDeviceUP(sender, deviceUPEvent)
                            End SyncLock

                            Exit Select

                        Case testEvents.TEST_DONE
                            '
                            '   sort the response
                            '
                            Dim resp As New RunTest.sResponse
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray,
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            If resp.rsp = response.SUCCESS Then

                                updateBar()
                                issueCmd_StopTest(qExecStatus.First)
                                executeTests(status)

                            End If
                            Exit Select

                    End Select

                    '   Event Switch Ends
                    Exit Select

                Case headerID.headerRequest
                    '
                    '   State machine for Request
                    '

                    '   Request Switch Ends
                    Exit Select

                Case headerID.headerResponse
                    '
                    '   State machine for Response
                    '
                    Select Case packetHeader.cmdid

                        Case commandIDs.TOOL_CMD_PREPARE_DUT_CNF
                            ' If devices are not up yet
                            If devicesUP = False Then
                                setStatusvarNull()
                                Exit Sub
                            End If
                            '   sort the response
                            Dim resp As New RunTest.sResponse
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray,
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            '   calib status
                            Dim c As New RunTest.sProdPrepRfStatusCnf
                            Dim r(Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf))) As Byte

                            Array.Copy(packet, (Marshal.SizeOf(GetType(RunTest.frmHeader)) + Marshal.SizeOf(GetType(RunTest.sResponse))),
                                       r, 0, Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf)))

                            c = CType(ByteToStruct(r, GetType(RunTest.sProdPrepRfStatusCnf)), RunTest.sProdPrepRfStatusCnf)

                            If resp.rsp = response.SUCCESS Then

                                If qExecStatus.Count = 0 Then
                                    setStatusvarNull()
                                    Exit Select
                                End If

                                updateBar()
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Or qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_START_TEST_REF

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Or qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_PREPARE_REFERENCE

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Or qExecStatus.First = tests.TEST_ID_RF_TX_SWEEP Then

                                    status.test = tests.TEST_ID_RF_TX
                                    status.state = RunTest.states.STATE_START_TEST_REF

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Or qExecStatus.First = tests.TEST_ID_RF_RX_SWEEP Then

                                    status.test = tests.TEST_ID_RF_RX
                                    status.state = RunTest.states.STATE_PREPARE_REFERENCE

                                End If
                                executeTests(status)

                            End If

                            If resp.rsp = response.FAILED Then
                                If c.calStatus = RFCalibStatus.RF_CALIBRATION_FAILED Then
                                    MsgBox("DUT Manual RF Calibration Required", MsgBoxStyle.Critical)
                                    enableUI()
                                    clearBar()
                                    setStatusvarNull()
                                    btn_ShowResults.Enabled = False

                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing
                                    testRunning = False

                                    ' clear test parameter queues
                                    sweepParamList.Clear()
                                    RFChannelParamList.Clear()
                                End If
                            End If
                            Exit Select

                        Case commandIDs.TOOL_CMD_PREPARE_REFERENCE_CNF
                            ' If devices are not up yet
                            If devicesUP = False Then
                                setStatusvarNull()
                                Exit Sub
                            End If

                            '   sort the response
                            Dim resp As New RunTest.sResponse
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray,
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            '   calib status
                            Dim c As New RunTest.sProdPrepRfStatusCnf
                            Dim r(Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf))) As Byte

                            Array.Copy(packet, (Marshal.SizeOf(GetType(RunTest.frmHeader)) + Marshal.SizeOf(GetType(RunTest.sResponse))),
                                       r, 0, Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf)))

                            c = CType(ByteToStruct(r, GetType(RunTest.sProdPrepRfStatusCnf)), RunTest.sProdPrepRfStatusCnf)

                            If resp.rsp = response.SUCCESS Then

                                If qExecStatus.Count = 0 Then
                                    setStatusvarNull()
                                    Exit Select
                                End If

                                updateBar()
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Or qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_PREPARE_DUT

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Or qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_START_TEST_DUT

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Or qExecStatus.First = tests.TEST_ID_RF_TX_SWEEP Then

                                    status.test = tests.TEST_ID_RF_TX
                                    status.state = RunTest.states.STATE_PREPARE_DUT

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Or qExecStatus.First = tests.TEST_ID_RF_RX_SWEEP Then

                                    status.test = tests.TEST_ID_RF_RX
                                    status.state = RunTest.states.STATE_START_TEST_DUT

                                End If
                                executeTests(status)

                            End If

                            If resp.rsp = response.FAILED Then

                                If c.calStatus = RFCalibStatus.RF_CALIBRATION_FAILED Then
                                    MsgBox("REF Manual RF Calibration Required", MsgBoxStyle.Critical)
                                    enableUI()
                                    clearBar()
                                    setStatusvarNull()
                                    btn_ShowResults.Enabled = False

                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing
                                    testRunning = False

                                    ' clear test parameter queues
                                    sweepParamList.Clear()
                                    RFChannelParamList.Clear()
                                End If
                            End If

                            Exit Select

                        Case commandIDs.TOOL_CMD_START_TEST_CNF
                            '   sort the response
                            Dim resp As New RunTest.sResponse
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray,
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            If resp.rsp = response.SUCCESS Then

                                If qExecStatus.Count = 0 Then
                                    setStatusvarNull()
                                    Exit Select
                                End If

                                updateBar()
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Or qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    If sender.mDevType = RunTest.ClientType.REF Then
                                        status.test = tests.TEST_ID_PLC_TX
                                        status.state = RunTest.states.STATE_START_TEST_DUT
                                        executeTests(status)
                                    End If

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Or qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    If sender.mDevType = RunTest.ClientType.DUT Then
                                        status.test = tests.TEST_ID_PLC_RX
                                        status.state = RunTest.states.STATE_START_TEST_REF
                                        executeTests(status)
                                    End If

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Or qExecStatus.First = tests.TEST_ID_RF_TX_SWEEP Then

                                    If sender.mDevType = RunTest.ClientType.REF Then
                                        status.test = tests.TEST_ID_RF_TX
                                        status.state = RunTest.states.STATE_START_TEST_DUT
                                        executeTests(status)
                                    End If

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Or qExecStatus.First = tests.TEST_ID_RF_RX_SWEEP Then

                                    If sender.mDevType = RunTest.ClientType.DUT Then
                                        status.test = tests.TEST_ID_RF_RX
                                        status.state = RunTest.states.STATE_START_TEST_REF
                                        executeTests(status)
                                    End If

                                End If
                            End If
                            Exit Select

                        Case commandIDs.TOOL_CMD_STOP_TEST_CNF
                            '   sort the response
                            Dim resp As New RunTest.sResponse
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray,
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            If resp.rsp = response.SUCCESS Then

                                If qExecStatus.Count = 0 Then
                                    setStatusvarNull()
                                    Exit Select
                                End If

                                updateBar()
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Or qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_TX

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Or qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_RX

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Or qExecStatus.First = tests.TEST_ID_RF_TX_SWEEP Then

                                    status.test = tests.TEST_ID_RF_TX

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Or qExecStatus.First = tests.TEST_ID_RF_RX_SWEEP Then

                                    status.test = tests.TEST_ID_RF_RX

                                End If
                                status.state = RunTest.states.STATE_GET_RESULT
                                executeTests(status)

                            End If
                            Exit Select

                        Case commandIDs.TOOL_CMD_GET_RESULT_CNF

                            UpdateDuration()
                            '   sort the response
                            Dim statsArray As New List(Of Byte)
                            Dim intf As RunTest.TestInterface = RunTest.get_interface(qExecStatus.First)
                            m.intf = intf

                            '   Result of PLC interface test
                            If intf = RunTest.TestInterface.TEST_PLC_ID Then
                                For Each b As Byte In packet.ToList.GetRange(Marshal.SizeOf(GetType(RunTest.frmHeader)),
                                                                             Marshal.SizeOf(GetType(shpgpStats._plcTxTestResults_t)))
                                    statsArray.Add(b)
                                    If statsArray.Count = Marshal.SizeOf(GetType(shpgpStats._plcTxTestResults_t)) Then
                                        Exit For
                                    End If
                                Next

                                Dim resp As New shpgpStats._plcTxTestResults_t
                                resp = CType(ByteToStruct(statsArray.ToArray,
                                                                   GetType(shpgpStats._plcTxTestResults_t)), shpgpStats._plcTxTestResults_t)

                                '   set metadata
                                m.rsp = resp
                            End If

                            '   Result of RF interface test
                            If intf = RunTest.TestInterface.TEST_802_15_5_ID Then
                                For Each b As Byte In packet.ToList.GetRange(Marshal.SizeOf(GetType(RunTest.frmHeader)),
                                                                             Marshal.SizeOf(GetType(shpgpStats._sRfStats)))
                                    statsArray.Add(b)
                                    If statsArray.Count = Marshal.SizeOf(GetType(shpgpStats._sRfStats)) Then
                                        Exit For
                                    End If
                                Next

                                Dim resp As New shpgpStats._sRfStats
                                resp = CType(ByteToStruct(statsArray.ToArray, GetType(shpgpStats._sRfStats)), shpgpStats._sRfStats)

                                '   set metadata
                                m.rf_rsp = resp
                            End If

                            updatemetadata()

                            Dim result = New shpgpStats
                            result.logResultinTextFile(m)

                            updateBar()
                            If qExecStatus.Count > 0 Then
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Then
                                    qExecStatus.RemoveAt(0)
                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing
                                    testRunning = False
                                    If qExecStatus.Count = 0 Then
                                        lineSent = False
                                        enableUI()
                                        ' if all tests are done
                                        'MessageBox.Show("all tests done")
                                        tests_done_update_summary(result)
                                    End If
                                    clearBar()
                                    executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then
                                    qExecStatus.RemoveAt(0)
                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing
                                    testRunning = False
                                    If qExecStatus.Count = 0 Then
                                        lineSent = False
                                        enableUI()
                                        ' if all tests are done
                                        'MessageBox.Show("all tests done")
                                        tests_done_update_summary(result)
                                    End If
                                    clearBar()
                                    executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    sweepParamList.Dequeue()
                                    swpCount += 1
                                    '   Stop Sweep Test
                                    If sweepParamList.Count = 0 Then

                                        '   if high power mode is done then send low power mode frames
                                        If setPowerMode = True Then

                                            setPowerMode = False

                                            status.test = tests.TEST_ID_PLC_TX_SWEEP
                                            status.state = RunTest.states.STATE_PREPARE_REFERENCE

                                            start_time = Now
                                            clearBar()
                                            executeTests(status)
                                            Exit Select

                                        End If

                                        qExecStatus.RemoveAt(0)

                                        setDefaultPLCPramas()
                                        sweepParamList.Clear()
                                        swpCount = 0

                                        start_time = Nothing
                                        stop_time = Nothing
                                        elapsed_time = Nothing

                                        testRunning = False
                                        If qExecStatus.Count = 0 Then
                                            lineSent = False
                                            enableUI()
                                            ' if all tests are done
                                            'MessageBox.Show("all tests done")
                                            tests_done_update_summary(result)
                                        End If
                                        sweepTestRunning = False
                                        'setPowerMode = True    ' Because we are using only low power mode

                                        clearBar()
                                        status.test = tests.TEST_ID_PLC_TX
                                        status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    Else
                                        status.test = tests.TEST_ID_PLC_TX
                                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                                        gtxTest = sweepParamList.Peek
                                        start_time = Now
                                    End If

                                    executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    sweepParamList.Dequeue()
                                    swpCount += 1
                                    '   Stop Sweep Test
                                    If sweepParamList.Count = 0 Then

                                        '   if high power mode is done then send low power mode frames
                                        If setPowerMode = True Then

                                            setPowerMode = False

                                            status.test = tests.TEST_ID_PLC_RX_SWEEP
                                            status.state = RunTest.states.STATE_PREPARE_DUT

                                            start_time = Now
                                            clearBar()
                                            executeTests(status)
                                            Exit Select

                                        End If

                                        qExecStatus.RemoveAt(0)

                                        testRunning = False
                                        If qExecStatus.Count = 0 Then
                                            lineSent = False
                                            enableUI()
                                            ' if all tests are done
                                            'MessageBox.Show("all tests done")
                                            tests_done_update_summary(result)
                                        End If
                                        sweepTestRunning = False
                                        'setPowerMode = True    ' Because we are using only low power mode

                                        setDefaultPLCPramas()
                                        sweepParamList.Clear()
                                        swpCount = 0

                                        start_time = Nothing
                                        stop_time = Nothing
                                        elapsed_time = Nothing

                                        clearBar()
                                        status.test = tests.TEST_ID_PLC_RX
                                        status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    Else
                                        status.test = tests.TEST_ID_PLC_RX
                                        status.state = RunTest.states.STATE_PREPARE_DUT

                                        gtxTest = sweepParamList.Peek
                                        start_time = Now
                                    End If

                                    executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Then

                                    qExecStatus.RemoveAt(0)
                                    status.test = tests.TEST_ID_RF_TX
                                    status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing
                                    testRunning = False
                                    If qExecStatus.Count = 0 Then
                                        lineSent = False
                                        enableUI()
                                        ' if all tests are done
                                        'MessageBox.Show("all tests done")
                                        tests_done_update_summary(result)
                                    End If
                                    clearBar()
                                    executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Then

                                    qExecStatus.RemoveAt(0)
                                    status.test = tests.TEST_ID_RF_RX
                                    status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing
                                    testRunning = False
                                    If qExecStatus.Count = 0 Then
                                        lineSent = False
                                        enableUI()
                                        ' if all tests are done
                                        'MessageBox.Show("all tests done")
                                        tests_done_update_summary(result)
                                    End If
                                    clearBar()
                                    executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_TX_SWEEP Then

                                    RFChannelParamList.Dequeue()
                                    swpCount += 1
                                    '   Stop Sweep Test
                                    If RFChannelParamList.Count = 0 Then

                                        qExecStatus.RemoveAt(0)

                                        testRunning = False
                                        If qExecStatus.Count = 0 Then
                                            lineSent = False
                                            enableUI()
                                            ' if all tests are done
                                            'MessageBox.Show("all tests done")
                                            tests_done_update_summary(result)
                                        End If
                                        sweepTestRunning = False

                                        setDefaultRFPramas()
                                        RFChannelParamList.Clear()
                                        swpCount = 0

                                        start_time = Nothing
                                        stop_time = Nothing
                                        elapsed_time = Nothing

                                        clearBar()
                                        status.test = tests.TEST_ID_RF_TX
                                        status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    Else
                                        status.test = tests.TEST_ID_RF_TX
                                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                                        rfgtxTest = RFChannelParamList.Peek
                                        start_time = Now
                                    End If

                                    executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_RF_RX_SWEEP Then

                                    RFChannelParamList.Dequeue()
                                    swpCount += 1
                                    '   Stop Sweep Test
                                    If RFChannelParamList.Count = 0 Then

                                        qExecStatus.RemoveAt(0)

                                        testRunning = False
                                        If qExecStatus.Count = 0 Then
                                            lineSent = False
                                            enableUI()
                                            ' if all tests are done
                                            'MessageBox.Show("all tests done")
                                            tests_done_update_summary(result)
                                        End If
                                        sweepTestRunning = False

                                        setDefaultRFPramas()
                                        RFChannelParamList.Clear()
                                        swpCount = 0

                                        start_time = Nothing
                                        stop_time = Nothing
                                        elapsed_time = Nothing

                                        clearBar()
                                        status.test = tests.TEST_ID_RF_RX
                                        status.state = RunTest.states.STATE_GET_RESULT_CNF
                                    Else
                                        status.test = tests.TEST_ID_RF_RX
                                        status.state = RunTest.states.STATE_PREPARE_DUT

                                        rfgtxTest = RFChannelParamList.Peek
                                        start_time = Now
                                    End If

                                    executeTests(status)
                                Else
                                    ' if all tests are done
                                    'MessageBox.Show("all tests done")
                                    'tests_done_update_summary(result)
                                End If
                            End If
                            Exit Select

                        Case commandIDs.TOOL_CMD_DEVICE_FLASH_PARAM_CNF
                            'MsgBox("Flash Done", MsgBoxStyle.Information)
                            lbl_flashDone.Text = "Flash Done"
                            lbl_flashDone.ForeColor = Color.ForestGreen
                            If txtbxSerialNum.TextLength >= RunTest.SR_NO_SIZE Then
                                Dim temp_txtbx_string As String = txtbxSerialNum.Text
                                txtbxSerialNum.Text = temp_txtbx_string.Trim().Remove(temp_txtbx_string.Length - 4)
                            End If
                            board_disconnect_cmd()
                            panel_disconnect_info.Visible = True
                            'btn_panel_ready.Visible = True
                            'Dim diagResult As MsgBoxResult = MsgBox("Boards Disconnected. Replace DUT", MessageBoxButtons.OK)
                            'If (diagResult = MsgBoxResult.Ok) Then

                            'End If
                            Exit Select
                    End Select
                    '   Response Switch Ends
                    Exit Select
            End Select
            'executeTests(status)
        Else
            MessageBox.Show("Incorrect Protocol")
        End If

    End Sub

#End Region

    '
    '   Update Test Duration
    '
    Private Sub UpdateDuration()
        stop_time = Now
        elapsed_time = stop_time.Subtract(start_time)
    End Sub

    '
    '   Decide if test passed or failed
    '
    Private Sub decideTestStatus()

        Dim p = New Decimal
        Dim f = New Double

        Dim od As Boolean = False   ' This variable will check if received frames are more than transmitted 

        '   As per the test interface, calculate the percentage of frames
        If m.intf = RunTest.TestInterface.TEST_PLC_ID Then

            If m.testPramas.frmType = TestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU Then
                If m.rsp.RxGoodDataCnt > m.testPramas.numFrames Then
                    od = True
                Else
                    f = (((m.testPramas.numFrames - m.rsp.RxGoodDataCnt) / m.testPramas.numFrames) * 100)
                End If
            ElseIf m.testPramas.frmType = TestSettings.eFrmType.HPGP_HW_FRMTYPE_MGMT Then
                If m.rsp.RxGoodMgmtCnt > m.testPramas.numFrames Then
                    od = True
                Else
                    f = (((m.testPramas.numFrames - m.rsp.RxGoodMgmtCnt) / m.testPramas.numFrames) * 100)
                End If
            End If
            dumpMsg(m.name & vbNewLine & "Frame Length - " & m.testPramas.frmLen)

        ElseIf m.intf = RunTest.TestInterface.TEST_802_15_5_ID Then

            If m.rf_rsp.rx_count > m.rftestPramas.frameCount Then
                od = True
            Else
                f = (((m.rftestPramas.frameCount - m.rf_rsp.rx_count) / m.rftestPramas.frameCount) * 100)
            End If
            dumpMsg(m.name & vbNewLine & "Channel - " & m.rftestPramas.ch.ToString("X"))

        End If

        '   Round off the percentage
        If Math.Round(f, 1, MidpointRounding.AwayFromZero) > Math.Round(f, 1, MidpointRounding.ToEven) Then
            p = Math.Round(f) + 1
        Else
            p = Math.Round(f)
        End If

        '   Compare the threshold as per interface
        If m.intf = RunTest.TestInterface.TEST_PLC_ID Then

            If p <= plcTestThreshold Then
                m.testStatus = True
            Else
                m.testStatus = False
            End If

        ElseIf m.intf = RunTest.TestInterface.TEST_802_15_5_ID Then

            If p <= rfTestThreshold Then
                m.testStatus = True
            Else
                m.testStatus = False
            End If

        End If

        ' This is a temp fix
        ' This is needed because sometimes, FW sends incorrect stats for the test
        ' e.g. The number of frames received is more than number of frames transmitted.
        ' This fix needs to be done in FW
        If od = True Then
            m.testStatus = True
        End If

        If m.testStatus = True Then
            dumpMsg("Status" & " : " & "PASS" & vbNewLine)
        ElseIf m.testStatus = False Then
            dumpMsg("Status" & " : " & "FAIL" & vbNewLine)
        End If

        ' Save the test result of the board in the test list of summary
        Dim x As New variations
        x.name = m.name
        x.result = m.testStatus
        s.tests.Add(x)
    End Sub

    '
    '   Update metadata
    '
    Private Sub updatemetadata()

        '   Assign lot number and serial number
        If Not txtbxLotNum.Text.Length = 0 Then
            m.lotnumber = txtbxLotNum.Text
        Else
            m.lotnumber = "0"
            s.lotnumber = "0"
        End If

        If Not txtbxSerialNum.Text.Length = 0 Then
            m.serialNum = txtbxSerialNum.Text
        Else
            m.serialNum = "0"
            s.serialNum = "0"
        End If

        m.startTime = start_time
        m.stopTime = stop_time
        m.duration = elapsed_time.ToString

        m.runCount = runCount

        If m.intf = RunTest.TestInterface.TEST_PLC_ID Then
            m.testPramas = gtxTest
            m.threshold = plcTestThreshold
        ElseIf m.intf = RunTest.TestInterface.TEST_802_15_5_ID Then
            m.rftestPramas = rfgtxTest
            m.threshold = rfTestThreshold
        End If

        If lineSent = False Then
            m.newTestRun = True
            lineSent = True
        Else
            m.newTestRun = False
        End If

        For Each cl As ConnectedClient In getSelectedClientList()
            If cl.mDevType = RunTest.ClientType.DUT Then
                m.DUT = cl.mClient.Client.RemoteEndPoint.ToString.Split(":"c).First
            ElseIf cl.mDevType = RunTest.ClientType.REF Then
                m.REF = cl.mClient.Client.RemoteEndPoint.ToString.Split(":"c).First
            End If
        Next

        If qExecStatus.First = tests.TEST_ID_PLC_TX Then
            m.name = "PLC Transmit Test"
        ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then
            m.name = "PLC Receive Test"
        ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then
            m.name = "PLC Transmit Sweep Test - " & "Variation - " & swpCount.ToString
        ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then
            m.name = "PLC Receive Sweep Test - " & "Variation - " & swpCount.ToString
        ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Then
            m.name = "RF Transmit Test"
        ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Then
            m.name = "RF Receive Test"
        ElseIf qExecStatus.First = tests.TEST_ID_RF_TX_SWEEP Then
            m.name = "RF Transmit Sweep Test - " & "Variation - " & swpCount.ToString
        ElseIf qExecStatus.First = tests.TEST_ID_RF_RX_SWEEP Then
            m.name = "RF Receive Sweep Test - " & "Variation - " & swpCount.ToString
        End If

        '   Assign lot number and serial number
        Dim lotSummaryPath As String = rootFilePath
        ' if root directory does not exist then create it
        If (Not System.IO.Directory.Exists(rootFilePath)) Then
            System.IO.Directory.CreateDirectory(rootFilePath)
        Else
            ' if root directory exists then check if lot number directory already exists
            ' if not then create it and then check if lot summary directory exists, if not 
            ' then create and log the results in it
            'Dim lotPath = System.IO.Path.Combine(rootFilePath, "Lot " & txtbxLotNum.Text.ToString)
            s.filepath = System.IO.Path.Combine(rootFilePath, "Lot " & txtbxLotNum.Text.ToString)
            If (Not System.IO.Directory.Exists(s.filepath)) Then
                System.IO.Directory.CreateDirectory(s.filepath)
            End If

            lotSummaryPath = System.IO.Path.Combine(s.filepath, "Lot Summary")
            If (Not System.IO.Directory.Exists(lotSummaryPath)) Then
                System.IO.Directory.CreateDirectory(lotSummaryPath)
            End If
        End If
        m.filePath = System.IO.Path.Combine(lotSummaryPath, m.serialNum.ToString & ".txt")

        decideTestStatus()
    End Sub

    '
    '   Update summary of the test
    '
    Private Sub updatesummary()

        '   Assign lot number and serial number
        If txtbxLotNum.Text.Length <> 0 Then
            s.lotnumber = txtbxLotNum.Text

            ' if root directory does not exist then create it
            If (Not System.IO.Directory.Exists(rootFilePath)) Then
                System.IO.Directory.CreateDirectory(rootFilePath)
            Else
                ' if root directory exists then check if lot number directory already exists
                ' if not then create it
                's.filepath = System.IO.Path.Combine(rootFilePath, "Lot " & txtbxLotNum.Text.ToString)
                If (Not System.IO.Directory.Exists(s.filepath)) Then
                    System.IO.Directory.CreateDirectory(s.filepath)
                End If

            End If

        End If

        If Not txtbxSerialNum.Text.Length = 0 Then
            Dim serialNum As String = ""
            serialNum = txtbxSerialNum.Text
            serialNum = serialNum.Replace("-", "")
            s.serialNum = serialNum
            If serialNum.Length <> (RunTest.SR_NO_SIZE - RunTest.SR_NO_TRIM_LEN) Then
                MsgBox("Please correct Serial No. and rerun test. Eg. xxxx-xxx-0-xxxxxxxx")
                txtbxSerialNum.Focus()
                Exit Sub
            End If
            ' Final test result for the board
            Dim finalResult As Boolean = True
            For Each t As variations In s.tests
                If t.result = False Then
                    finalResult = False
                    Exit For
                End If
            Next

            If finalResult = True Then
                s.finalResult = "PASS"
                lblResult.ForeColor = Color.ForestGreen
                lblResult.Text = "BOARD PASS"
                'SetSysColors(3, COLOR_WINDOWFRAME, RGB(0, 255, 0))
                pbox_test_status.SizeMode = PictureBoxSizeMode.StretchImage
                pbox_test_status.Image = My.Resources.test_result_success

                ' convert MAC address to string
                If Not txtbxSerialNum.Text = "" Then
                    ' Dim macAddressLastBytes(3) As Byte
                    Dim tempString As String = ""
                    Const OUI_INDEX As Byte = 7
                    'Array.Clear(macAddressLastBytes, 0, 3)
                    'serialNum = txtbxSerialNum.Text
                    'serialNum = serialNum.Replace("-", "")
                    ' Sr No. PNDS-MYY-0-XXXXXXXXX - Product Number,Device Type, Sub Device Type ()
                    ' 1st X = Encoded OUI
                    ' 84:86:f3 encoded as 0 in Serial Number. OUI Field is indexed at location 7

                    If serialNum.Chars(OUI_INDEX) = "0" Then
                        txtbx_MACAddr.Text = ""
                        txtbx_MACAddr.Text = "84:86:F3"
                    Else
                        MsgBox("Invalid Sr. No or OUI Not Supported. Eg. xxxx-xxx-0-xxxxxxxx")
                        Exit Sub
                    End If

                    Try
                        tempString = Mid(serialNum, 8, 9) ' For Mid function index starts from 1. Not from 8 like others

                        Dim macaddressPart As String = CLng(tempString).ToString("X06") ' represents last 3 bytes of mac address in hex characters
                        Dim colonOffset As Integer = 2
                        Dim sbString As StringBuilder = New StringBuilder(macaddressPart)

                        While True
                            sbString.Insert(colonOffset, ":")
                            colonOffset += 3
                            If colonOffset >= sbString.Length Then
                                Exit While
                            End If
                        End While
                        txtbx_MACAddr.AppendText(":" & sbString.ToString())
                        'tempString = Mid(serialNum, 9, 3)
                        'macAddressLastBytes(0) = CByte(tempString)
                        'txtbx_MACAddr.AppendText(":" & macAddressLastBytes(0).ToString("X02"))

                        'tempString = Mid(serialNum, 12, 3)
                        'macAddressLastBytes(1) = CByte(tempString)
                        'txtbx_MACAddr.AppendText(":" & macAddressLastBytes(1).ToString("X02"))

                        'tempString = Mid(serialNum, 15, 3)
                        'macAddressLastBytes(2) = CByte(tempString)
                        'txtbx_MACAddr.AppendText(":" & macAddressLastBytes(2).ToString("X02"))

                        Dim str1 As String = txtbx_MACAddr.Text.Replace(":", "")
                        SyncLock objForMac
                            gMACAddress = ULong.Parse(str1, NumberStyles.HexNumber, CultureInfo.CurrentCulture.NumberFormat)
                        End SyncLock
                    Catch e As Exception
                        MsgBox("Invalid Sr. No: Hex Conversion failed. Eg. xxxx-xxx-0-valid numbers")
                        Exit Sub
                    End Try
                    lbl_flashDone.Text = "Flash Pending"
                    lbl_flashDone.ForeColor = Color.DarkRed
                    SyncLock objForMac
                        s.MAC = txtbx_MACAddr.Text
                        gMACcounter = gMACAddress

                    End SyncLock

                    flashParams()
                    'Dim Path = IO.Path.Combine(rootFilePath, "MAC_Addr_Log.xml")
                    ' Dim xmlWrite As New readConfig
                    ' xmlWrite.create_LogMACAddr_XML_file(Path, txtbx_MACAddr.Text)

                End If

            Else    ' If any test is failed, do not assign any MAC address
                s.finalResult = "FAIL"
                s.MAC = "N/A"
                lblResult.ForeColor = Color.DarkRed
                'SetSysColors(3, COLOR_WINDOWFRAME, RGB(255, 0, 0))
                lblResult.Text = "BOARD FAIL"
                pbox_test_status.SizeMode = PictureBoxSizeMode.StretchImage
                pbox_test_status.Image = My.Resources.test_result_error
                panel_disconnect_info.Visible = True
                If txtbxSerialNum.TextLength >= RunTest.SR_NO_SIZE Then
                    Dim temp_txtbx_string As String = txtbxSerialNum.Text
                    txtbxSerialNum.Text = temp_txtbx_string.Trim().Remove(temp_txtbx_string.Length - 4)
                End If

            End If

            ' send MAC address to device

        End If

    End Sub

    Private Sub flashParams()

        Dim finalResult As Boolean = True
        If Not IsNothing(s.tests) Then
            For Each test As variations In s.tests
                If test.result = False Then
                    finalResult = False
                    Exit For
                End If
            Next
        Else
            MsgBox("Tests are not ready/performed or boards are not available")
            Exit Sub
        End If
        If finalResult = True Then
            Dim t As HomeScreen.tests
            t = status.test
            For Each cl As ConnectedClient In getSelectedClientList()

                If cl.mDevType = RunTest.ClientType.DUT Then
                    'If sweepTestRunning = False Then
                    'if this is not rf sweep test then assign only one set of params
                    'rfgtxTest.ch = RF_CHANNEL
                    'End If
                    'swap_dest_short_address()
                    If Not txtbxSerialNum.Text = "" Then
                        Dim serialNum As String = ""
                        serialNum = txtbxSerialNum.Text
                        serialNum = serialNum.Replace("-", "")

                        If serialNum.Length = (RunTest.SR_NO_SIZE - RunTest.SR_NO_TRIM_LEN) Then
                            RunTest.rftestParams = rfgtxTest
                            lbl_flashDone.Text = "Flash write in process"
                            lbl_flashDone.ForeColor = Color.DarkRed
                            RunTest.beginSend(RunTest.states.STATE_DEVICE_FLASH_PARAMS, cl, t)
                        Else
                            MsgBox("Invalid Serial No. Eg. xx-xxxxxx-0-xxxxxxxxx")
                        End If
                    Else
                        MsgBox("Invalid Serial No. Eg. xx-xxxxxx-0-xxxxxxxxx")
                    End If
                End If

            Next
        Else
            MsgBox("Cannot flash as test not performed or failed")
        End If

    End Sub
    '
    '   All tests are done and now update the summary 
    '
    Private Sub tests_done_update_summary(result As shpgpStats)
        ''   if all tests are done then log device summary
        updatesummary()
        result.logSumary(s)
        If s.finalResult = "PASS" Then
            result.serialToMAC_map(s)
        End If
    End Sub

    '
    '   Disable UI functions while running tests
    '
    Private Sub disableUI()

        ' disable text boxes
        txtbxLotNum.Enabled = False
        txtbxSerialNum.Enabled = False

        btn_ScanDevices.Enabled = False
        btnStopTest.Enabled = True

        btnRunTest.Enabled = False
        btn_ShowResults.Enabled = False
        btn_SetIP.Enabled = False

        chkbx_PLCRXSweep.Enabled = False
        btn_PLCRXSweepSettings.Enabled = False
        chkbx_PLCTXSweep.Enabled = False
        btn_PLCTXSweepSettings.Enabled = False

        chkbxPLCRX.Enabled = False
        btnPLCRXSettings.Enabled = False
        chkbxPLCTX.Enabled = False
        btnPLCTXSettings.Enabled = False

        chkbxRFTX.Enabled = False
        btnRFTXSetting.Enabled = False
        chkbxRFRX.Enabled = False
        btnRFRXSetting.Enabled = False

        chkbx_RFTXSweep.Enabled = False
        btn_RFTXSweepSetting.Enabled = False
        chkbx_RFRXSweep.Enabled = False
        btn_RFRXSweepSetting.Enabled = False
        btn_flashParams.Enabled = False
    End Sub

    '
    '   Enable UI functions disabled earlier during the test
    '
    Private Sub enableUI()

        ' enable text boxes
        txtbxLotNum.Enabled = True
        txtbxSerialNum.Enabled = True

        btn_ScanDevices.Enabled = True
        btnStopTest.Enabled = False

        btnRunTest.Enabled = True
        btn_ShowResults.Enabled = True
        btn_SetIP.Enabled = True

        chkbx_PLCRXSweep.Enabled = True
        btn_PLCRXSweepSettings.Enabled = True
        chkbx_PLCTXSweep.Enabled = True
        btn_PLCTXSweepSettings.Enabled = True

        chkbxPLCRX.Enabled = True
        btnPLCRXSettings.Enabled = True
        chkbxPLCTX.Enabled = True
        btnPLCTXSettings.Enabled = True

        chkbxRFTX.Enabled = True
        btnRFTXSetting.Enabled = True
        chkbxRFRX.Enabled = True
        btnRFRXSetting.Enabled = True

        chkbx_RFTXSweep.Enabled = True
        btn_RFTXSweepSetting.Enabled = True
        chkbx_RFRXSweep.Enabled = True
        btn_RFRXSweepSetting.Enabled = True
        btn_flashParams.Enabled = True

        chkbxRFTX.Checked = False
        chkbxRFRX.Checked = False

        chkbxPLCTX.Checked = False
        chkbxPLCRX.Checked = False
    End Sub

    '   <summary>
    '           State machines for sweep tests where default parameters for the tests are set
    '   </summary>
#Region "Sweep Test"

    '
    '   Set the default parameters for the PLC tests
    '
    Private Sub setDefaultPLCPramas()

        gtxTest.snid = 1
        gtxTest.dtei = 2
        gtxTest.stei = 3

        gtxTest.ermode = TestSettings.erModestate.erModeON
        gtxTest.txpowermode = TestSettings.eTxPwrMode.AUTOMOTIVE_TX_POWER_MODE

        gtxTest.descLen = TestSettings.HYBRII_CELLBUF_SIZE

        gtxTest.secTestMode = TestSettings.eSecTestMode.ENCRYPTED
        gtxTest.eks = 0

        gtxTest.frmType = TestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU

        gtxTest.lenTestMode = TestSettings.eLenTestMode.FIXED_LEN
        gtxTest.mcstMode = TestSettings.eFrmMcstMode.HPGP_MCST

        '   Can be used for sweep test
        gtxTest.frmLen = 100
        gtxTest.delay = 10
        gtxTest.numFrames = 1000

    End Sub

    '
    '   Configure PLC sweep parameters
    '
    Private Sub sweepTest()

        swpCount = 1

        For Each len As UInteger In FrmLenArr

            Dim temp_gtxtest As New TestSettings._sPlcSimTxTestParams
            temp_gtxtest = gtxTest

            If len <= 100 Then
                temp_gtxtest.ermode = TestSettings.erModestate.erModeON
            Else
                temp_gtxtest.ermode = TestSettings.erModestate.erModeOFF
            End If

            temp_gtxtest.frmLen = len

            If setPowerMode = False Then
                temp_gtxtest.txpowermode = TestSettings.eTxPwrMode.AUTOMOTIVE_TX_POWER_MODE
            End If

            temp_gtxtest.frmType = TestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU
            temp_gtxtest.plid = TestSettings.eHpgpPlidValue.HPGP_PLID0
            temp_gtxtest.secTestMode = TestSettings.eSecTestMode.ENCRYPTED
            temp_gtxtest.eks = 0
            temp_gtxtest.delay = 4

            sweepParamList.Enqueue(temp_gtxtest)

        Next

    End Sub

    '
    '   Vary frame length
    '
    Private Sub varyFrmLen()

        Dim temp_gtxtest As New TestSettings._sPlcSimTxTestParams
        temp_gtxtest = gtxTest

        For Each l As UInteger In FrmLenArr
            If l <= 100 Then
                temp_gtxtest.ermode = TestSettings.erModestate.erModeON
            Else
                temp_gtxtest.ermode = TestSettings.erModestate.erModeOFF
            End If
            temp_gtxtest.frmLen = l
            If setPowerMode = False Then
                temp_gtxtest.txpowermode = TestSettings.eTxPwrMode.AUTOMOTIVE_TX_POWER_MODE
            End If
            temp_gtxtest.frmType = TestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU
            temp_gtxtest.plid = TestSettings.eHpgpPlidValue.HPGP_PLID0
            temp_gtxtest.secTestMode = TestSettings.eSecTestMode.ENCRYPTED
            temp_gtxtest.eks = 0
            temp_gtxtest.delay = 4
            sweepParamList.Enqueue(temp_gtxtest)
        Next

    End Sub

    '
    '   Configure RF sweep parameters
    '
    Private Sub RFChannelSweep()

        varyRFChannel()
        swpCount = 1

    End Sub

    '
    '   Vary channel for RF sweep parameters
    '
    Private Sub varyRFChannel()

        Dim temp As New TestSettings.sRfTxTestParams
        temp = rfgtxTest

        For Each b As Byte In RFChannelList
            temp.ch = b
            RFChannelParamList.Enqueue(temp)
        Next

    End Sub

    '
    '   Configure default parameters for RF TX
    '
    Private Sub setDefaultRFPramas()

        ReDim rfgtxTest.macAddress(8)
        '   macAddress
        rfgtxTest.macAddress(0) = &H0
        rfgtxTest.macAddress(1) = &H0
        rfgtxTest.macAddress(2) = &H84
        rfgtxTest.macAddress(3) = &H86
        rfgtxTest.macAddress(4) = &HF3
        rfgtxTest.macAddress(5) = &H0
        rfgtxTest.macAddress(6) = &HAB
        rfgtxTest.macAddress(7) = &HCD

        '   short addresses
        rfgtxTest.macShortAddress = &H1122
        rfgtxTest.dstShortAddress = &H3344

        'rfgtxTest.ch = &HB
        rfgtxTest.panId = &H2222
        rfgtxTest.frameLength = 102
        rfgtxTest.frameCount = RF_FRM_NUM
        rfgtxTest.interFrameDelay = 20

    End Sub

    '
    '   Swap source and destination short address
    '
    Private Sub swap_dest_short_address()

        Dim temp As UShort
        '   macAddress
        rfgtxTest.macAddress(0) = &H0
        rfgtxTest.macAddress(1) = &H0
        rfgtxTest.macAddress(2) = &H84
        rfgtxTest.macAddress(3) = &H86
        rfgtxTest.macAddress(4) = &HF3
        rfgtxTest.macAddress(5) = &H0
        rfgtxTest.macAddress(6) = &HDC
        rfgtxTest.macAddress(7) = &HBA

        '   short addresses
        temp = rfgtxTest.macShortAddress
        rfgtxTest.macShortAddress = rfgtxTest.dstShortAddress
        rfgtxTest.dstShortAddress = temp

    End Sub

#End Region

    '   <summary>
    '           State machines for individual tests
    '   </summary>
#Region "Execution Subs"

    '
    '   Set name of the ongoing test
    '
    Private Sub set_session_test_name(ByRef status As execState)
        Select Case status.test
            Case tests.TEST_ID_PLC_RX
                lblSessionName.Text = chkbxPLCRX.Text
                Exit Select

            Case tests.TEST_ID_PLC_RX_SWEEP
                lblSessionName.Text = chkbx_PLCRXSweep.Text
                Exit Select

            Case tests.TEST_ID_PLC_TX
                lblSessionName.Text = chkbxPLCTX.Text
                Exit Select

            Case tests.TEST_ID_PLC_TX_SWEEP
                lblSessionName.Text = chkbx_PLCTXSweep.Text
                Exit Select

            Case tests.TEST_ID_RF_RX
                lblSessionName.Text = chkbxRFRX.Text
                Exit Select

            Case tests.TEST_ID_RF_TX
                lblSessionName.Text = chkbxRFTX.Text
                Exit Select

            Case tests.TEST_ID_RF_RX_SWEEP
                lblSessionName.Text = chkbx_RFRXSweep.Text
                Exit Select

            Case tests.TEST_ID_RF_TX_SWEEP
                lblSessionName.Text = chkbx_RFTXSweep.Text
                Exit Select

        End Select
    End Sub

    '
    '   Run the test
    '
    Private Sub btnRunTest_Click(sender As Object, e As EventArgs) Handles btnRunTest.Click

        Dim serialNum As String = ""
        Dim stringValid As String = ""
        txtbx_MACAddr.Text = "xx:xx:xx:xx:xx:xx"
        serialNum = txtbxSerialNum.Text
        serialNum = serialNum.Replace("-", "")
        s.serialNum = serialNum
        If serialNum.Length <> (RunTest.SR_NO_SIZE - RunTest.SR_NO_TRIM_LEN) Then
            MsgBox("Please correct Serial No. & rerun test. Eg. xxxx-xxx-0-xxxxxxxx or xxxx-xxx-0xxxxxxxx")
            txtbxSerialNum.Focus()
            Exit Sub
        End If
        stringValid = Mid(serialNum, 9, 8)
        If Not System.Text.RegularExpressions.Regex.IsMatch(stringValid, "^[0-9]+$") Then
            MsgBox("Please correct Serial No. & rerun test. Eg. xxxx-xxx-0-0numbers or xxxx-xxx-0numbers")
            txtbxSerialNum.Focus()
            Exit Sub
        End If
        s = New summary
        s.tests = New List(Of variations)

        If serialNum.Chars(0) <> BOARD_TYPE_GV7011_MOD Then 'Enable only PLC TX RX with 100 bytes & no sweep test
            Select Case DUTCapbility
                Case capability.RF_PLC

                    enablePLCRX()
                    enablePLCTX()
                    'If chkbx_PLCRXSweep.Checked = False Then
                    disablePLCRXSweep()
                    'End If
                    ' If chkbx_PLCTXSweep.Checked = False Then
                    disablePLCTXSweep()
                    ' End If

                Case Else
            End Select
        Else ' Do PLC Sweep test
            Select Case DUTCapbility
                Case capability.RF_PLC

                    enablePLCRXSweep()
                    enablePLCTXSweep()
                    ' If chkbxPLCRX.Checked = False Then
                    disablePLCRX()
                    ' End If
                    '  If chkbxPLCTX.Checked = False Then
                    disablePLCTX()
                    '  End If

                Case Else
            End Select
        End If

        SyncLock objForUI
            txtbxDummy.Clear()
            lblResult.Text = String.Empty
        End SyncLock
        'lbl_flashDone.Text = "Do Not Flash"
        'lbl_flashDone.ForeColor = Color.DarkRed
        setStatusvarNull()

        If getSelectedClientList().Count <= 1 Then
            MessageBox.Show("Select more than one devices and Restart the test !!")
            scan()
            Exit Sub
        End If


        If qExecStatus.Count = 0 Then
            MessageBox.Show("Select atleast one test !!")
            Exit Sub
        End If

        runCount += 1
        testRunning = True
        pbox_test_status.Image = Nothing
        disableUI()

        ' Tests to be executed
        Select Case qExecStatus.First
            ' PLC
            Case tests.TEST_ID_PLC_TX
                status.test = tests.TEST_ID_PLC_TX
                status.state = RunTest.states.STATE_PREPARE_REFERENCE
                Exit Select

            Case tests.TEST_ID_PLC_RX
                status.test = tests.TEST_ID_PLC_RX
                status.state = RunTest.states.STATE_PREPARE_DUT
                Exit Select

            Case tests.TEST_ID_PLC_TX_SWEEP
                status.test = tests.TEST_ID_PLC_TX_SWEEP
                status.state = RunTest.states.STATE_PREPARE_REFERENCE
                Exit Select

            Case tests.TEST_ID_PLC_RX_SWEEP
                status.test = tests.TEST_ID_PLC_RX_SWEEP
                status.state = RunTest.states.STATE_PREPARE_DUT
                Exit Select

                ' RF
            Case tests.TEST_ID_RF_TX
                status.test = tests.TEST_ID_RF_TX
                status.state = RunTest.states.STATE_PREPARE_REFERENCE
                Exit Select

            Case tests.TEST_ID_RF_TX_SWEEP
                status.test = tests.TEST_ID_RF_TX_SWEEP
                status.state = RunTest.states.STATE_PREPARE_REFERENCE
                Exit Select

            Case tests.TEST_ID_RF_RX
                status.test = tests.TEST_ID_RF_RX
                status.state = RunTest.states.STATE_PREPARE_DUT
                Exit Select

            Case tests.TEST_ID_RF_RX_SWEEP
                status.test = tests.TEST_ID_RF_RX_SWEEP
                status.state = RunTest.states.STATE_PREPARE_DUT
                Exit Select

            Case Else
                MessageBox.Show("Incorrect Test Id")
                Exit Sub
        End Select

        start_time = Now
        dumpMsg("*************************")    '25 *
        dumpMsg("Starting tests...")
        executeTests(status)

    End Sub

    '
    '   Sequenced tests
    '
    Public Sub executeTests(ByVal status As HomeScreen.execState)

        set_session_test_name(status)

        Select Case status.test

            Case tests.TEST_ID_PLC_TX

                _test_plcTX(status)

                Exit Select

            Case tests.TEST_ID_PLC_RX

                _test_plcRX(status)

                Exit Select

            Case tests.TEST_ID_PLC_TX_SWEEP

                _test_plcTXSweep(status)

                Exit Select

            Case tests.TEST_ID_PLC_RX_SWEEP

                _test_plcRXSweep(status)

                Exit Select

            Case tests.TEST_ID_RF_TX

                _test_RFTX(status)

                Exit Select

            Case tests.TEST_ID_RF_TX_SWEEP

                _test_RFTXSweep(status)

                Exit Select

            Case tests.TEST_ID_RF_RX

                _test_RFRX(status)

                Exit Select

            Case tests.TEST_ID_RF_RX_SWEEP

                _test_RFRXSweep(status)

                Exit Select

        End Select

    End Sub

    '   <summary>
    '             RF TX Sub
    '   </summary>
    Private Sub _test_RFTX(ByVal status As execState)

        Dim t As HomeScreen.tests
        t = status.test

        Select Case status.state

            Case RunTest.states.STATE_PREPARE_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        If sweepTestRunning = False Then
                            'if this is not rf sweep test then assign only one set of params
                            rfgtxTest.ch = RF_CHANNEL
                        End If
                        swap_dest_short_address()
                        RunTest.rftestParams = rfgtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_DUT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_PREPARE_REFERENCE

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        If sweepTestRunning = False Then
                            'if this is not rf sweep test then assign only one set of params
                            rfgtxTest.ch = RF_CHANNEL
                        End If
                        RunTest.rftestParams = rfgtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_REFERENCE, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_REF

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_STOP_TEST

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_STOP_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_GET_RESULT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT_CNF

                clearBar()
                If qExecStatus.Count > 0 Then
                    status.test = qExecStatus.First

                    If qExecStatus.First = tests.TEST_ID_PLC_TX Then
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_TX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_RX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Then

                        status.test = tests.TEST_ID_RF_TX
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Then

                        status.test = tests.TEST_ID_RF_RX
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    End If

                    start_time = Now
                    executeTests(status)

                Else
                    ' If all tests are done then based DUT capability 
                    ' enable the tests to simulate cyclic manner
                    Select Case DUTCapbility
                        Case capability.PLC
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                        Case capability.RF
                            enableRFTXSweep()
                            enableRFRXSweep()
                        Case capability.RF_PLC
                            enableRFTXSweep()
                            enableRFRXSweep()

                            'enablePLCTXSweep()
                            'enablePLCRXSweep()
                            'disablePLCRXSweep()
                            'disablePLCTXSweep()
#If LED_BOARD_TEST = "YES" Then
                            enablePLCRX()
                            enablePLCTX()
                            disablePLCRXSweep()
                            disablePLCTXSweep()

#ElseIf LED_BOARD_TEST = "NO" Then
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                            disablePLCRX()
                            disablePLCTX()
#Else

#End If
                        Case Else
                    End Select

                End If
                Exit Select

            Case Else
                status.state = RunTest.states.STATE_PREPARE_REFERENCE
                Exit Select

        End Select

    End Sub

    '   <summary>
    '             RF RX Sub
    '   </summary>
    Private Sub _test_RFRX(ByVal status As execState)

        Dim t As HomeScreen.tests
        t = status.test

        Select Case status.state

            Case RunTest.states.STATE_PREPARE_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        If sweepTestRunning = False Then
                            'if this is not rf sweep test then assign only one set of params
                            rfgtxTest.ch = RF_CHANNEL
                        End If
                        RunTest.rftestParams = rfgtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_DUT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_PREPARE_REFERENCE

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        If sweepTestRunning = False Then
                            'if this is not rf sweep test then assign only one set of params
                            rfgtxTest.ch = RF_CHANNEL
                        End If
                        swap_dest_short_address()
                        RunTest.rftestParams = rfgtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_REFERENCE, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_REF

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_STOP_TEST

                For Each cl As ConnectedClient In getSelectedClientList()

                    '   because in RF RX test REF is transmitter and DUT is receiver
                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_STOP_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_GET_RESULT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT_CNF

                clearBar()
                If qExecStatus.Count > 0 Then
                    status.test = qExecStatus.First

                    If qExecStatus.First = tests.TEST_ID_PLC_TX Then
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_TX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_RX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Then

                        status.test = tests.TEST_ID_RF_TX
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Then

                        status.test = tests.TEST_ID_RF_RX
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    End If

                    start_time = Now
                    executeTests(status)

                Else
                    ' If all tests are done then based DUT capability 
                    ' enable the tests to simulate cyclic manner
                    Select Case DUTCapbility
                        Case capability.PLC
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                        Case capability.RF
                            enableRFTXSweep()
                            enableRFRXSweep()
                        Case capability.RF_PLC
                            enableRFTXSweep()
                            enableRFRXSweep()
#If LED_BOARD_TEST = "YES" Then
                            enablePLCRX()
                            enablePLCTX()
                            disablePLCRXSweep()
                            disablePLCTXSweep()
#ElseIf LED_BOARD_TEST = "NO" Then
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                            disablePLCRX()
                            disablePLCTX()
#Else
#End If
                            'disablePLCRXSweep()
                            'disablePLCTXSweep()
                        Case Else
                    End Select

                End If
                Exit Select

            Case Else
                status.state = RunTest.states.STATE_PREPARE_DUT
                Exit Select

        End Select

    End Sub

    '   <summary>
    '             RF TX SWEEP Sub
    '   </summary>
    Private Sub _test_RFTXSweep(ByVal status As execState)

        RFChannelSweep()

        status.test = tests.TEST_ID_RF_TX
        status.state = RunTest.states.STATE_PREPARE_REFERENCE

        rfgtxTest = RFChannelParamList.First
        sweepTestRunning = True

        dumpMsg("RF Transmit Sweep")
        dumpMsg("-------------------------")    '25 Dashes

        _test_RFTX(status)

    End Sub

    '   <summary>
    '             RF RX SWEEP Sub
    '   </summary>
    Private Sub _test_RFRXSweep(ByVal status As execState)

        RFChannelSweep()

        status.test = tests.TEST_ID_RF_RX
        status.state = RunTest.states.STATE_PREPARE_DUT

        rfgtxTest = RFChannelParamList.First
        sweepTestRunning = True

        dumpMsg("RF Receive Sweep")
        dumpMsg("-------------------------")    '25 Dashes

        _test_RFRX(status)

    End Sub

    '   <summary>
    '               PLC TX Sub
    '   </summary>
    Private Sub _test_plcTX(ByVal status As execState)

        Dim t As HomeScreen.tests
        t = status.test

        Select Case status.state

            Case RunTest.states.STATE_PREPARE_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        If sweepTestRunning = False And userPLCTXPramasSet = True Then
                            gtxTest = plcTXparams
                        End If
                        RunTest.gtxTest = gtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_DUT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_PREPARE_REFERENCE

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        If sweepTestRunning = False And userPLCTXPramasSet = True Then
                            gtxTest = plcTXparams
                        End If
                        RunTest.gtxTest = gtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_REFERENCE, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_REF

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_STOP_TEST

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_STOP_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_GET_RESULT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT_CNF

                clearBar()
                If qExecStatus.Count > 0 Then
                    status.test = qExecStatus.First

                    If qExecStatus.First = tests.TEST_ID_PLC_TX Then
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_TX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_RX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Then

                        status.test = tests.TEST_ID_RF_TX
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Then

                        status.test = tests.TEST_ID_RF_RX
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    End If

                    start_time = Now
                    executeTests(status)

                Else
                    ' If all tests are done then based DUT capability 
                    ' enable the tests to simulate cyclic manner
                    Select Case DUTCapbility
                        Case capability.PLC
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                        Case capability.RF
                            enableRFTXSweep()
                            enableRFRXSweep()
                        Case capability.RF_PLC
                            enableRFTXSweep()
                            enableRFRXSweep()
                            'enablePLCTXSweep()
                            'enablePLCRXSweep()
#If LED_BOARD_TEST = "YES" Then
                            enablePLCRX()
                            enablePLCTX()
                            disablePLCRXSweep()
                            disablePLCTXSweep()
#ElseIf LED_BOARD_TEST = "NO" Then
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                            disablePLCRX()
                            disablePLCTX()
#Else
#End If
                        Case Else
                    End Select

                End If
                Exit Select

            Case Else
                status.state = RunTest.states.STATE_PREPARE_REFERENCE
                Exit Select

        End Select

    End Sub

    '   <summary>
    '               PLC RX Sub
    '   </summary>
    Private Sub _test_plcRX(ByVal status As execState)

        Dim t As HomeScreen.tests
        t = status.test

        Select Case status.state

            Case RunTest.states.STATE_PREPARE_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        If sweepTestRunning = False And userPLCRXPramasSet = True Then
                            gtxTest = plcRXparams
                        End If
                        RunTest.gtxTest = gtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_DUT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_PREPARE_REFERENCE

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        If sweepTestRunning = False And userPLCRXPramasSet = True Then
                            gtxTest = plcRXparams
                        End If
                        RunTest.gtxTest = gtxTest
                        RunTest.beginSend(RunTest.states.STATE_PREPARE_REFERENCE, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_DUT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_START_TEST_REF

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.REF Then
                        RunTest.beginSend(RunTest.states.STATE_START_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_STOP_TEST

                For Each cl As ConnectedClient In getSelectedClientList()

                    '   because in PLC RX test REF is transmitter and DUT is receiver
                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_STOP_TEST, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT

                For Each cl As ConnectedClient In getSelectedClientList()

                    If cl.mDevType = RunTest.ClientType.DUT Then
                        RunTest.beginSend(RunTest.states.STATE_GET_RESULT, cl, t)
                    End If

                Next
                updateBar()
                Exit Select

            Case RunTest.states.STATE_GET_RESULT_CNF

                clearBar()
                If qExecStatus.Count > 0 Then
                    status.test = qExecStatus.First

                    If qExecStatus.First = tests.TEST_ID_PLC_TX Then
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_TX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                        status.test = tests.TEST_ID_PLC_RX_SWEEP
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_TX Then

                        status.test = tests.TEST_ID_RF_TX
                        status.state = RunTest.states.STATE_PREPARE_REFERENCE

                    ElseIf qExecStatus.First = tests.TEST_ID_RF_RX Then

                        status.test = tests.TEST_ID_RF_RX
                        status.state = RunTest.states.STATE_PREPARE_DUT

                    End If

                    start_time = Now
                    executeTests(status)

                Else
                    ' If all tests are done then based DUT capability 
                    ' enable the tests to simulate cyclic manner
                    Select Case DUTCapbility
                        Case capability.PLC
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                        Case capability.RF
                            enableRFTXSweep()
                            enableRFRXSweep()
                        Case capability.RF_PLC
                            enableRFTXSweep()
                            enableRFRXSweep()
                            'enablePLCTXSweep()
                            'enablePLCRXSweep()
#If LED_BOARD_TEST = "YES" Then
                            enablePLCRX()
                            enablePLCTX()
                            disablePLCRXSweep()
                            disablePLCTXSweep()

#ElseIf LED_BOARD_TEST = "NO" Then
                            enablePLCTXSweep()
                            enablePLCRXSweep()
                            disablePLCRX()
                            disablePLCTX()
#Else
#End If
                        Case Else
                    End Select

                End If

                Exit Select

            Case Else
                status.state = RunTest.states.STATE_PREPARE_DUT
                Exit Select

        End Select
    End Sub

    '   <summary>
    '               PLC TX SWEEP Sub
    '   </summary>
    Private Sub _test_plcTXSweep(ByVal status As execState)

        sweepTest()

        Select Case gtxTest.txpowermode
            Case TestSettings.eTxPwrMode.AUTOMOTIVE_TX_POWER_MODE
                dumpMsg("Auto Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case TestSettings.eTxPwrMode.DEFAULT_TX_POWER_MODE
                dumpMsg("Default Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case TestSettings.eTxPwrMode.HIGH_TX_POWER_MODE
                dumpMsg("High Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case TestSettings.eTxPwrMode.NORMAL_TX_POWER_MODE
                dumpMsg("Normal Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case Else
        End Select

        status.test = tests.TEST_ID_PLC_TX
        status.state = RunTest.states.STATE_PREPARE_REFERENCE

        gtxTest = sweepParamList.First
        sweepTestRunning = True

        _test_plcTX(status)

    End Sub

    '   <summary>
    '               PLC RX SWEEP Sub
    '   </summary>
    Private Sub _test_plcRXSweep(ByVal status As execState)

        sweepTest()

        Select Case gtxTest.txpowermode
            Case TestSettings.eTxPwrMode.AUTOMOTIVE_TX_POWER_MODE
                dumpMsg("Auto Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case TestSettings.eTxPwrMode.DEFAULT_TX_POWER_MODE
                dumpMsg("Default Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case TestSettings.eTxPwrMode.HIGH_TX_POWER_MODE
                dumpMsg("High Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case TestSettings.eTxPwrMode.NORMAL_TX_POWER_MODE
                dumpMsg("Normal Power Mode")
                dumpMsg("-------------------------")    '25 Dashes
            Case Else
        End Select

        status.test = tests.TEST_ID_PLC_RX
        status.state = RunTest.states.STATE_PREPARE_DUT

        gtxTest = sweepParamList.First
        sweepTestRunning = True

        _test_plcRX(status)

    End Sub

#End Region

    '   <summary>
    '           Checkbox and settings button events
    '           Revisit this logic to simplify things
    '   </summary>
#Region "Wrapper for button events"

#Region "Enable"
    '
    '   Enable RF TX
    '
    Private Sub enableRFTX()
        chkbxRFTX.Checked = False
        chkbxRFTX.Checked = True
        btnRFTXSetting.Enabled = True
    End Sub

    '
    '   Enable RF TX SWEEP
    '
    Private Sub enableRFTXSweep()
        chkbx_RFTXSweep.Checked = False
        chkbx_RFTXSweep.Checked = True
        btn_RFTXSweepSetting.Enabled = True
    End Sub

    '
    '   Enable RF RX
    '
    Private Sub enableRFRX()
        chkbxRFRX.Checked = False
        chkbxRFRX.Checked = True
        btnRFRXSetting.Enabled = True
    End Sub

    '
    '   Enable RF RX SWEEP
    '
    Private Sub enableRFRXSweep()
        chkbx_RFRXSweep.Checked = False
        chkbx_RFRXSweep.Checked = True
        btn_RFRXSweepSetting.Enabled = True
    End Sub

    '
    '   Enable PLC TX
    '
    Private Sub enablePLCTX()
        chkbxPLCTX.Checked = False
        chkbxPLCTX.Checked = True
        btnPLCTXSettings.Enabled = True
    End Sub

    '
    '   Enable PLC TX SWEEP
    '
    Private Sub enablePLCTXSweep()
        chkbx_PLCTXSweep.Checked = False
        chkbx_PLCTXSweep.Checked = True
        btn_PLCTXSweepSettings.Enabled = True
    End Sub

    '
    '   Enable PLC RX
    '
    Private Sub enablePLCRX()
        chkbxPLCRX.Checked = False
        chkbxPLCRX.Checked = True
        btnPLCRXSettings.Enabled = True
    End Sub

    '
    '   Enable PLC TX SWEEP
    '
    Private Sub enablePLCRXSweep()
        chkbx_PLCRXSweep.Checked = False
        chkbx_PLCRXSweep.Checked = True
        btn_PLCRXSweepSettings.Enabled = True
    End Sub

#End Region

#Region "Disable"
    '
    '   Disable RF TX
    '
    Private Sub disableRFTX()
        chkbxRFTX.Checked = False
        btnRFTXSetting.Enabled = False
    End Sub

    '
    '   Disable RF TX SWEEP
    '
    Private Sub disableRFTXSweep()
        chkbx_RFTXSweep.Checked = False
        btn_RFTXSweepSetting.Enabled = False
    End Sub

    '
    '   Disable RF RX
    '
    Private Sub disableRFRX()
        chkbxRFRX.Checked = False
        btnRFRXSetting.Enabled = False
    End Sub

    '
    '   Disable RF RX SWEEP
    '
    Private Sub disableRFRXSweep()
        chkbx_RFRXSweep.Checked = False
        btn_RFRXSweepSetting.Enabled = False
    End Sub

    '
    '   Disable PLC TX
    '
    Private Sub disablePLCTX()
        chkbxPLCTX.Checked = False
        btnPLCTXSettings.Enabled = False
    End Sub

    '
    '   Disable PLC TX SWEEP
    '
    Private Sub disablePLCTXSweep()
        chkbx_PLCTXSweep.Checked = False
        btn_PLCTXSweepSettings.Enabled = False
    End Sub

    '
    '   Disable PLC RX
    '
    Private Sub disablePLCRX()
        chkbxPLCRX.Checked = False
        btnPLCRXSettings.Enabled = False
    End Sub

    '
    '   Disable PLC TX SWEEP
    '
    Private Sub disablePLCRXSweep()
        chkbx_PLCRXSweep.Checked = False
        btn_PLCRXSweepSettings.Enabled = False
    End Sub

#End Region

#End Region

    '   <summary>
    '       Checkbox and settings button events
    '   </summary>
#Region "Checkbox and Settings Button Events"
    '
    '   PLC TX Sweep Test checkbox
    '
    Private Sub chkbx_txSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_PLCTXSweep.CheckedChanged

        If chkbx_PLCTXSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_PLC_TX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_PLC_TX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX))
                    chkbxPLCTX.Checked = False
                    btn_PLCTXSweepSettings.Enabled = False
                End If
            End If
            btn_PLCTXSweepSettings.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX_SWEEP))
            End If
        End If
    End Sub
    '
    '   PLC RX Sweep Test checkbox
    '
    Private Sub chkbx_rxSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_PLCRXSweep.CheckedChanged

        If chkbx_PLCRXSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_PLC_RX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX))
                    chkbxPLCRX.Checked = False
                    btn_PLCRXSweepSettings.Enabled = False
                End If
            End If
            btn_PLCRXSweepSettings.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX_SWEEP))
            End If
        End If
    End Sub
    '
    '   PLC TX Test parameter selection
    '
    Private Sub chkbxSPITX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxPLCTX.CheckedChanged
        '   Set status test
        If chkbxPLCTX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX) Or qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_PLC_TX)
            End If
            btnPLCTXSettings.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX))
            End If
        End If
    End Sub

    '
    '   PLC RX Test parameter selection
    '
    Private Sub chkbxSPIRX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxPLCRX.CheckedChanged
        '   Set status test
        If chkbxPLCRX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Or qExecStatus.Contains(tests.TEST_ID_PLC_RX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_PLC_RX)
            End If
            btnPLCRXSettings.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX))
            End If
        End If
    End Sub

    '
    '   RF TX Test enque
    '
    Private Sub chkbx_RFTX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxRFTX.CheckedChanged
        '   Set status test
        If chkbxRFTX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_TX) Or qExecStatus.Contains(tests.TEST_ID_RF_TX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_TX)
            End If
            btnRFTXSetting.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_TX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_TX))
            End If
        End If
    End Sub

    '
    '   RF RX Test enque
    '
    Private Sub chkbx_RFRX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxRFRX.CheckedChanged
        '   Set status test
        If chkbxRFRX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_RX) Or qExecStatus.Contains(tests.TEST_ID_RF_RX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_RX)
            End If
            btnRFRXSetting.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_RX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_RX))
            End If
        End If

    End Sub

    '
    '   RF TX Sweep enque
    '
    Private Sub chkbxRFTXSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_RFTXSweep.CheckedChanged

        If chkbx_RFTXSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_TX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_TX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_RF_TX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_TX))
                    chkbxRFTX.Checked = False
                    btnRFTXSetting.Enabled = False
                End If
            End If
            btn_RFTXSweepSetting.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_TX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_TX_SWEEP))
            End If
        End If

    End Sub

    '
    '   RF RX Sweep enque
    '
    Private Sub chkbxRFRXSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_RFRXSweep.CheckedChanged

        If chkbx_RFRXSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_RX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_RX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_RF_RX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_RX))
                    chkbxRFRX.Checked = False
                    btnRFRXSetting.Enabled = False
                End If
            End If
            btn_RFRXSweepSetting.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_RX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_RX_SWEEP))
            End If
        End If

    End Sub

    '
    '   View Transmit Test settings
    '
    Private Sub btnSettingsSPITX_Click(sender As Object, e As EventArgs) Handles btnPLCTXSettings.Click
        testParamsFor = tests.TEST_ID_PLC_TX
        TestSettings.Show()
    End Sub

    Private Sub btnSettingsSPIRX_Click(sender As Object, e As EventArgs) Handles btnPLCRXSettings.Click
        testParamsFor = tests.TEST_ID_PLC_RX
        TestSettings.Show()
    End Sub

    Private Sub btn_txSweepSettings_Click(sender As Object, e As EventArgs) Handles btn_PLCTXSweepSettings.Click
        testParamsFor = tests.TEST_ID_PLC_TX
        TestSettings.Show()
    End Sub

    Private Sub btn_rxSweepSettings_Click(sender As Object, e As EventArgs) Handles btn_PLCRXSweepSettings.Click
        testParamsFor = tests.TEST_ID_PLC_RX
        TestSettings.Show()
    End Sub

#End Region

    '   <summary>
    '       Result log browsing
    '   </summary>
#Region "Result log location browsing"

    '
    '   Seesion location browsing
    '
    Private Sub btnBrowseLocation_Click(sender As Object, e As EventArgs)

        Dim newResultPath As String = String.Empty
        Using ofd As New OpenFileDialog
            If ofd.ShowDialog = Windows.Forms.DialogResult.OK Then
                ofd.Title = "Set Result File And Location"
                ofd.InitialDirectory = My.Computer.FileSystem.SpecialDirectories.MyDocuments
                ofd.Filter = "Text files (*.txt)|*.txt"
                ofd.RestoreDirectory = True
                newResultPath = ofd.FileName
            End If
        End Using

        If newResultPath = String.Empty Then
            m.filePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "Test Results.txt")
        Else
            m.filePath = newResultPath
        End If
        lblSessionLocation.Text = m.filePath

    End Sub

    '
    '   Open result file
    '
    Private Sub btn_ShowResults_Click(sender As Object, e As EventArgs) Handles btn_ShowResults.Click

        If System.IO.File.Exists(m.filePath) = True Then
            Process.Start(m.filePath)
        Else
            MessageBox.Show("File Does Not Exist")
        End If

    End Sub

#End Region

    '   <summary>
    '       Reset Devices | Unused
    '   </summary>
    Private Sub btn_ResetDevices_Click(sender As Object, e As EventArgs) Handles btn_ResetDevices.Click
        For Each cl As ConnectedClient In clients
            RunTest.beginSend(RunTest.states.STATE_DEVICE_RESET, cl, Nothing)
        Next
    End Sub

    '   <summary>
    '       Changing local machine IPv4 addres
    '   </summary>
#Region "Configure Local Machine IPv4 Address"

    '
    '   Get IP Address of local machine
    '
    Function GetIP() As String

        Return (
            From networkInterface In NetworkInterface.GetAllNetworkInterfaces()
            Where networkInterface.NetworkInterfaceType = NetworkInterfaceType.Ethernet
            From address In networkInterface.GetIPProperties().UnicastAddresses
            Where address.Address.AddressFamily = Net.Sockets.AddressFamily.InterNetwork
            Select ip = address.Address.ToString()
        ).FirstOrDefault()

    End Function

    '
    '   Change IP address of the Host PC
    '
    Private Sub Button2_Click(sender As Object, e As EventArgs) Handles btn_SetIP.Click
        'Changed the 3 IPs below
        Dim IPAddress As String = "192.168.1.101"
        Dim SubnetMask As String = "255.255.255.0"
        'Dim Gateway As String = "0.0.0.0"

        If GetIP() = IPAddress Then
            MessageBox.Show("Local Machine IPv4 Address already set!")
            'btn_SetIP.Enabled = False
            Exit Sub
        End If
        dumpMsg("Setting Local Machine IP Address to 192.168.1.101")

        Dim objMC As ManagementClass = New ManagementClass("Win32_NetworkAdapterConfiguration")
        Dim objMOC As ManagementObjectCollection = objMC.GetInstances()

        For Each objMO As ManagementObject In objMOC
            If (Not CBool(objMO("IPEnabled"))) Then
                Continue For
            End If

            Try
                Dim objNewIP As ManagementBaseObject = Nothing
                Dim objSetIP As ManagementBaseObject = Nothing
                Dim objNewGate As ManagementBaseObject = Nothing

                objNewIP = objMO.GetMethodParameters("EnableStatic")
                objNewGate = objMO.GetMethodParameters("SetGateways")

                'Set DefaultGateway
                'objNewGate("DefaultIPGateway") = New String() {Gateway}
                'objNewGate("GatewayCostMetric") = New Integer() {1}

                'Set IPAddress and Subnet Mask
                objNewIP("IPAddress") = New String() {IPAddress}
                objNewIP("SubnetMask") = New String() {SubnetMask}

                objSetIP = objMO.InvokeMethod("EnableStatic", objNewIP, Nothing)
                objSetIP = objMO.InvokeMethod("SetGateways", objNewGate, Nothing)

                'Changed this line so I could see if it was executing all of the way
                MessageBox.Show("Updated IPAddress, SubnetMask and Default Gateway!")

            Catch ex As Exception
                MessageBox.Show("Unable to Set IP : " & ex.Message)
            End Try
        Next objMO
    End Sub

#End Region

    '   <summary>
    '       These are utility functions used in tool
    '   </summary>
#Region "Utility Functions"

    '   <summary>
    '       Utility functions for progress bar
    '   </summary>
#Region "Progress Bar"

    Private Sub updateBar()
        Try
            SyncLock objForUI
                bar.Step = 10
                bar.PerformStep()

                If bar.Value >= bar.Maximum Then
                    bar.Value = bar.Maximum
                    System.Threading.Thread.Sleep(50)
                    bar.Value = bar.Minimum
                End If
            End SyncLock
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

    Private Sub clearBar()
        Try
            SyncLock objForUI
                bar.Value = bar.Maximum
                System.Threading.Thread.Sleep(30)
                bar.Value = bar.Minimum
            End SyncLock
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

#End Region

    '   <summary>
    '       Endieness converters | unused
    '   </summary>
#Region "Endieness Converters"

    ' 2-byte number
    Public Function SHORT_little_endian_TO_big_endian(i As Integer) As Integer
        Return ((i >> 8) And &HFF) + ((i << 8) And &HFF00)
    End Function

    ' 4-byte number
    Public Function INT_little_endian_TO_big_endian(i As Integer) As Integer
        Return ((i And &HFF) << 24) + ((i And &HFF00) << 8) + ((i And &HFF0000) >> 8) + ((i >> 24) And &HFF)
    End Function

#End Region

    '   <summary>
    '       Convert byte array to structure
    '   </summary>
    Public Function ByteToStruct(ByVal btData As Byte(), ByVal StructureType As Type) As Object
        Dim iStructSize As Integer = Marshal.SizeOf(StructureType)
        If (iStructSize > btData.Length) Then
            Return Nothing
        End If

        Dim buffer As IntPtr = Marshal.AllocHGlobal(iStructSize)
        Marshal.Copy(btData, 0, buffer, iStructSize)

        Dim retobj As New Object
        retobj = Marshal.PtrToStructure(buffer, StructureType)
        Marshal.FreeHGlobal(buffer)

        Return retobj
    End Function

    '   <summary>
    '       Dump messages on dummy text box
    '   </summary>
    Public Sub dumpMsg(ByVal msg As String)
        Try
            SyncLock objForUI
                With txtbxDummy
                    .AppendText(msg)
                    .ScrollToCaret()
                    .AppendText(vbNewLine)
                End With
            End SyncLock
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub
#End Region
    Private Sub btn_flashParams_Click(sender As Object, e As EventArgs) Handles btn_flashParams.Click

        Dim finalResult As Boolean = True
        If Not IsNothing(s.tests) Then
            For Each test As variations In s.tests
                If test.result = False Then
                    finalResult = False
                    Exit For
                End If
            Next
        Else
            MsgBox("Tests are not ready/performed or boards are not available")
            Exit Sub
        End If
        If finalResult = True Then
            Dim t As HomeScreen.tests
            t = status.test
            For Each cl As ConnectedClient In getSelectedClientList()

                If cl.mDevType = RunTest.ClientType.DUT Then
                    'If sweepTestRunning = False Then
                    'if this is not rf sweep test then assign only one set of params
                    'rfgtxTest.ch = RF_CHANNEL
                    'End If
                    'swap_dest_short_address()
                    If Not txtbxSerialNum.Text = "" Then
                        Dim serialNum As String = ""
                        serialNum = txtbxSerialNum.Text
                        serialNum = serialNum.Replace("-", "")

                        If serialNum.Length = (RunTest.SR_NO_SIZE - RunTest.SR_NO_TRIM_LEN) Then
                            RunTest.rftestParams = rfgtxTest
                            lbl_flashDone.Text = "Flash write in process"
                            lbl_flashDone.ForeColor = Color.DarkRed
                            RunTest.beginSend(RunTest.states.STATE_DEVICE_FLASH_PARAMS, cl, t)
                        Else
                            MsgBox("Invalid Serial No. Eg. xx-xxxxxx-0-xxxxxxxxx")
                        End If
                    Else
                        MsgBox("Invalid Serial No. Eg. xx-xxxxxx-0-xxxxxxxxx")
                    End If
                End If

            Next
        Else
            MsgBox("Cannot flash as test not performed or failed")
        End If

    End Sub
    Private Sub board_disconnect_cmd()
        Dim t As HomeScreen.tests
        t = status.test
        For Each cl As ConnectedClient In getSelectedClientList()
            If cl.mDevType = RunTest.ClientType.DUT Then
                RunTest.beginSend(RunTest.states.STATE_DEVICE_SPI_DISCONNECT, cl, t)
            End If
        Next
    End Sub
    Private Sub btn_board_disconnect_Click(sender As Object, e As EventArgs)
        board_disconnect_cmd()
    End Sub

    Private Sub btn_panel_ready_Click(sender As Object, e As EventArgs) Handles btn_panel_ready.Click
        panel_disconnect_info.Visible = False

        'btn_panel_ready.Visible = False
    End Sub
End Class
