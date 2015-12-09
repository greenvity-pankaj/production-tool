Imports System, System.Net, System.Net.Sockets, System.Windows.Forms
Imports System.Text, System.IO, System.Threading
Imports System.Collections, System.Collections.Generic, System.ComponentModel, System.Management
Imports System.Runtime.InteropServices
Imports System.Net.NetworkInformation

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

#End Region

    '
    '   Structure Declaration
    '
#Region "Structure Declaratio"
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
        Public testPramas As spiTXTestSettings._sPlcSimTxTestParams
        Public rsp As shpgpStats._plcTxTestResults_t
        Public rftestPramas As spiTXTestSettings.sRfTxTestParams
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
    Public userPLCTXPramasSet As New Boolean
    Public userPLCRXPramasSet As New Boolean
    Private sweepTestRunning As New Boolean

    '   Delegates
    Private Delegate Sub _initUIDelegate()
    Private Delegate Function getSelectedLVIItemDelegate()
    Private Delegate Sub _AddClient(ByVal client As Socket, ByVal type As RunTest.ClientType)

    '   Directory
    Private clDirectory = New Dictionary(Of String, ConnectedClient)

    '   Enum Variables
    Public testParamsFor As tests

    '   Duration Calculations
    Private stop_time As DateTime = Nothing
    Private start_time As DateTime = Nothing
    Private elapsed_time As TimeSpan = Nothing

    '   Integers Variables
    Private minVal As Decimal = 0
    Private maxVal As Decimal = 130
    Private port As Integer = 54321
    Private swpCount As New UInteger
    Private rfswpCount As New UInteger
    Private runCount As New UInteger
    Public threshold As UInteger = 10
    Private serverThreadCount As Integer = 0
    Private pendingConnections As Integer = 5
    Private RF_FRM_NUM As Integer = 200

    '   Lists
    Private sockets As List(Of Socket)
    Private clients As New List(Of ConnectedClient)
    Private qExecStatus As New List(Of tests)
    Private defTestQ As New List(Of tests)
    Private FrmLenArr As New List(Of UInteger)(New UInteger() {100, 500})
    Private RFChannelList As New List(Of Byte)(New Byte() {&HF, &H14, &H1A})

    '   Queue
    Private sweepParamList As New Queue(Of spiTXTestSettings._sPlcSimTxTestParams)
    Private RFChannelParamList As New Queue(Of spiTXTestSettings.sRfTxTestParams)

    '   Sockets
    Private serverStream As NetworkStream
    Private listener As System.Net.Sockets.TcpListener

    '   String Variables
    Private st As String
    Private filereader As String
    Public rootFilePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "Production Tool Logs")

    '   Structure Variables
    Public m As metadata
    Public s As New summary
    Public status As New execState
    Public plcRXparams As New spiTXTestSettings._sPlcSimTxTestParams
    Public plcTXparams As New spiTXTestSettings._sPlcSimTxTestParams
    Public gtxTest As New spiTXTestSettings._sPlcSimTxTestParams  'global variable to carry PLC test parameter
    Public rfgtxTest As New spiTXTestSettings.sRfTxTestParams  'global variable to carry RF test parameter

    '   Threads
    Private mainThread As Thread
    Private threadList As New List(Of Thread)
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
    Private Sub HomeScreen_Disposed(sender As Object, e As EventArgs) Handles Me.Disposed
        Try
            If Me.serverStream IsNot Nothing Then
                Me.serverStream.Close()
                Me.serverStream.Dispose()
            End If
            Me.mainThread.Abort()
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

    '
    '   Main form load
    '
    Private Sub Form1_Load(sender As Object, e As EventArgs) Handles MyBase.Load

        Control.CheckForIllegalCrossThreadCalls = False
        System.Net.ServicePointManager.Expect100Continue = False

        'Dim s As New readConfig
        's.readXML()

        mainThread = New Thread(AddressOf begin)
        mainThread.IsBackground = True
        mainThread.Priority = ThreadPriority.Normal
        mainThread.Start()

    End Sub

    '
    '   Run multiple threads
    '
    Private Sub begin()

        serverThreadCount = 1
        initUI()
        startServer()

    End Sub
    '
    '   Init all server threads
    '
    Public Function RunThreads(count As Integer, start As ThreadStart) As List(Of Thread)

        Dim list As New List(Of Thread)
        For i = 0 To count - 1
            Dim thread = New Thread(start)
            thread.Name = String.Format("Thread{0}", i + 1)
            thread.Start()
            list.Add(thread)
        Next
        Return list

    End Function

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
            btnSettingsPLCRX.Enabled = False
            'PLC TX
            chkbxPLCTX.Enabled = False
            btnSettingsPLCTX.Enabled = False
            'RF TX
            chkbx_RFTX.Enabled = False
            btnSettingRFTX.Enabled = False
            chkbxRFTXSweep.Enabled = False
            btnSettingRFTXsweep.Enabled = False
            'RF RX
            chkbx_RFRX.Enabled = False
            btnSettingRFRX.Enabled = False
            chkbxRFRXSweep.Enabled = False
            btnSettingRFRXsweep.Enabled = False

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
            lvClients.Items.Add(incomingClient.Client.RemoteEndPoint.ToString.Split(":"c).First)
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
                txtbxDummy.Clear()
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

                For Each cl As ConnectedClient In clients
                    If cl.mClient.Connected Then
                        RunTest.beginSend(RunTest.states.STATE_DEVICE_SEARCH, cl, Nothing)
                    Else
                        'cl.mClient.Client.Close()
                        'removeClient(cl)
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
            AddClient(sender.mClient.Client, RunTest.ClientType.REF)
            REF_UP = True
        End If

        If deviceUPEvent.deviceType = RunTest.ClientType.DUT Then
            sender.mDevType = deviceUPEvent.deviceType
            clDirectory(sender.mClient.Client.RemoteEndPoint.ToString.Split(":"c).First) = sender
            AddClient(sender.mClient.Client, RunTest.ClientType.DUT)
            DUT_UP = True
        End If

        If DUT_UP = True And REF_UP = True Then
            devicesUP = True
            enableTests2(deviceUPEvent.capabilityInfo)
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
                btnSettingsPLCTX.Enabled = True
                chkbxPLCRX.Enabled = True
                btnSettingsPLCRX.Enabled = True

                ' PLC TX Sweep
                chkbx_txSweep.Enabled = True
                chkbx_txSweep.Checked = True
                btn_txSweepSettings.Enabled = True
                ' PLC RX Sweep
                chkbx_rxSweep.Enabled = True
                chkbx_rxSweep.Checked = True
                btn_rxSweepSettings.Enabled = True
                ' Enable run tests button
                btnRunTest.Enabled = True
                Exit Select

            Case capability.RF_PLC
                ' PLC tests
                setDefaultPLCPramas()
                ' enable PLC TX & RX
                chkbxPLCTX.Enabled = True
                btnSettingsPLCTX.Enabled = True
                chkbxPLCRX.Enabled = True
                btnSettingsPLCRX.Enabled = True

                ' PLC TX Sweep
                chkbx_txSweep.Enabled = True
                chkbx_txSweep.Checked = True
                btn_txSweepSettings.Enabled = True
                ' PLC RX Sweep
                chkbx_rxSweep.Enabled = True
                chkbx_rxSweep.Checked = True
                btn_rxSweepSettings.Enabled = True

                ' RF tests
                setDefaultRFPramas()
                ' enable RF TX & RX
                chkbx_RFTX.Enabled = True
                btnSettingRFTX.Enabled = True
                chkbx_RFRX.Enabled = True
                btnSettingRFRX.Enabled = True

                ' RF TX Sweep Test
                chkbxRFTXSweep.Enabled = True
                chkbxRFTXSweep.Checked = True
                ' RF RX Sweep Test
                chkbxRFRXSweep.Enabled = True
                chkbxRFRXSweep.Checked = True

                ' Enable run tests button
                btnRunTest.Enabled = True
                Exit Select

            Case capability.RF
                ' RF tests
                setDefaultRFPramas()
                ' enable RF TX & RX
                chkbx_RFTX.Enabled = True
                btnSettingRFTX.Enabled = True
                chkbx_RFRX.Enabled = True
                btnSettingRFRX.Enabled = True

                ' RF TX Sweep Test
                chkbxRFTXSweep.Enabled = True
                chkbxRFTXSweep.Checked = True
                ' RF RX Sweep Test
                chkbxRFRXSweep.Enabled = True
                chkbxRFRXSweep.Checked = True

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
                        chkbx_txSweep.Enabled = True
                        chkbx_txSweep.Checked = True
                        btn_txSweepSettings.Enabled = True
                        ' PLC RX Sweep
                        chkbx_rxSweep.Enabled = True
                        chkbx_rxSweep.Checked = True
                        btn_rxSweepSettings.Enabled = True
                        ' Enable run tests button
                        btnRunTest.Enabled = True
                        Exit Select

                    Case bitmap.RF_TX

                        setDefaultRFPramas()

                        chkbx_RFTX.Enabled = True
                        chkbx_txSweep.Checked = True
                        chkbx_rxSweep.Checked = True
                        btn_rxSweepSettings.Enabled = True
                        btn_txSweepSettings.Enabled = True
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
                            Dim upEventArray As New List(Of Byte)
                            For Each b As Byte In packet.ToList.GetRange(Marshal.SizeOf(GetType(RunTest.frmHeader)), _
                                                                         Marshal.SizeOf(GetType(RunTest.sDevUpEvent)))
                                upEventArray.Add(b)
                                If upEventArray.Count = Marshal.SizeOf(GetType(RunTest.sDevUpEvent)) Then
                                    Exit For
                                End If
                            Next

                            Dim deviceUPEvent As New RunTest.sDevUpEvent
                            deviceUPEvent = CType(ByteToStruct(upEventArray.ToArray, _
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
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray, _
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
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray, _
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)


                            'For Each x As Byte In packet
                            '    dumpMsg(x)
                            'Next

                            '   calib status
                            Dim c As New RunTest.sProdPrepRfStatusCnf
                            Dim r(Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf))) As Byte

                            Array.Copy(packet, (Marshal.SizeOf(GetType(RunTest.frmHeader)) + Marshal.SizeOf(GetType(RunTest.sResponse))), r, 0, Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf)))

                            c = CType(ByteToStruct(r, GetType(RunTest.sProdPrepRfStatusCnf)), RunTest.sProdPrepRfStatusCnf)

                            If resp.rsp = response.SUCCESS Then

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
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray, _
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)


                            'For Each x As Byte In packet
                            '    dumpMsg(x)
                            'Next

                            '   calib status
                            Dim c As New RunTest.sProdPrepRfStatusCnf
                            Dim r(Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf))) As Byte

                            Array.Copy(packet, (Marshal.SizeOf(GetType(RunTest.frmHeader)) + Marshal.SizeOf(GetType(RunTest.sResponse))), r, 0, Marshal.SizeOf(GetType(RunTest.sProdPrepRfStatusCnf)))

                            c = CType(ByteToStruct(r, GetType(RunTest.sProdPrepRfStatusCnf)), RunTest.sProdPrepRfStatusCnf)

                            If resp.rsp = response.SUCCESS Then

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
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray, _
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            If resp.rsp = response.SUCCESS Then

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
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray, _
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            If resp.rsp = response.SUCCESS Then

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
                                For Each b As Byte In packet.ToList.GetRange(Marshal.SizeOf(GetType(RunTest.frmHeader)), _
                                                                             Marshal.SizeOf(GetType(shpgpStats._plcTxTestResults_t)))
                                    statsArray.Add(b)
                                    If statsArray.Count = Marshal.SizeOf(GetType(shpgpStats._plcTxTestResults_t)) Then
                                        Exit For
                                    End If
                                Next

                                Dim resp As New shpgpStats._plcTxTestResults_t
                                resp = CType(ByteToStruct(statsArray.ToArray, _
                                                                   GetType(shpgpStats._plcTxTestResults_t)), shpgpStats._plcTxTestResults_t)

                                '   set metadata
                                m.rsp = resp
                            End If

                            '   Result of RF interface test
                            If intf = RunTest.TestInterface.TEST_802_15_5_ID Then
                                For Each b As Byte In packet.ToList.GetRange(Marshal.SizeOf(GetType(RunTest.frmHeader)), _
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

                            'decideTestStatus()
                            updatemetadata()

                            Dim result = New shpgpStats
                            result.logResultinTextFile(m)

                            updateBar()

                            ''   If any test fails
                            'If m.testStatus = False Then
                            '    qExecStatus.Clear()
                            '    sweepParamList.Clear()

                            '    status = Nothing
                            '    start_time = Nothing
                            '    stop_time = Nothing
                            '    elapsed_time = Nothing

                            '    testRunning = False
                            '    lineSent = False
                            '    enableUI()
                            '    Exit Sub
                            'End If

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

        Dim od As Boolean = False   ' This variable will check if received frames are more than transmitted in case of RF

        If m.intf = RunTest.TestInterface.TEST_PLC_ID Then

            If m.rsp.TotalRxGoodFrmCnt > m.testPramas.numFrames Then
                od = True
            Else
                f = (((m.testPramas.numFrames - m.rsp.TotalRxGoodFrmCnt) / m.testPramas.numFrames) * 100)
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

        If Math.Round(f, 1, MidpointRounding.AwayFromZero) > Math.Round(f, 1, MidpointRounding.ToEven) Then
            p = Math.Round(f) + 1
        Else
            p = Math.Round(f)
        End If

        If p <= threshold Then
            m.testStatus = True
        Else
            m.testStatus = False
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
        's = New summary
        's.tests = New List(Of variations)
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
        m.threshold = threshold

        If m.intf = RunTest.TestInterface.TEST_PLC_ID Then
            m.testPramas = gtxTest
        ElseIf m.intf = RunTest.TestInterface.TEST_802_15_5_ID Then
            m.rftestPramas = rfgtxTest
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
    Public gMACcounter = New ULong
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

                'Dim r = New shpgpStats
                'r.createSummaryLog(s)

            End If

        End If

        If Not txtbxSerialNum.Text.Length = 0 Then
            s.serialNum = txtbxSerialNum.Text

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
                lblResult.Text = "BOARD PASS"

                ' convert MAC address to string
                'Dim t As String = Hex(gMACcounter)
                'Dim i As Integer = 2
                'Dim sb As StringBuilder = New StringBuilder(t)
                'While True
                '    sb.Insert(i, ":")
                '    i += 3
                '    If i >= sb.Length Then
                '        Exit While
                '    End If
                'End While
                's.MAC = sb.ToString

                ' increment MAC counter
                gMACcounter += 1

            Else
                s.finalResult = "FAIL"
                s.MAC = String.Empty
                lblResult.Text = "BOARD FAIL"
            End If

            ' send MAC address to device

        End If

    End Sub

    '
    '   All tests are done and now update the summary 
    '
    Private Sub tests_done_update_summary(result As shpgpStats)
        ''   if all tests are done then log device summary
        'dumpMsg("calling >>> updateSummary")
        updatesummary()
        'result.logSumary(s)
        'If s.finalResult = "PASS" Then
        '    result.serialToMAC_map(s)
        'End If
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

        chkbx_rxSweep.Enabled = False
        btn_rxSweepSettings.Enabled = False
        chkbx_txSweep.Enabled = False
        btn_txSweepSettings.Enabled = False

        chkbxPLCRX.Enabled = False
        btnSettingsPLCRX.Enabled = False
        chkbxPLCTX.Enabled = False
        btnSettingsPLCTX.Enabled = False

        chkbx_RFTX.Enabled = False
        btnSettingRFTX.Enabled = False
        chkbx_RFRX.Enabled = False
        btnSettingRFRX.Enabled = False

        chkbxRFTXSweep.Enabled = False
        btnSettingRFTXsweep.Enabled = False
        chkbxRFRXSweep.Enabled = False
        btnSettingRFRXsweep.Enabled = False

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

        chkbx_rxSweep.Enabled = True
        btn_rxSweepSettings.Enabled = True
        chkbx_txSweep.Enabled = True
        btn_txSweepSettings.Enabled = True

        chkbxPLCRX.Enabled = True
        btnSettingsPLCRX.Enabled = True
        chkbxPLCTX.Enabled = True
        btnSettingsPLCTX.Enabled = True

        chkbx_RFTX.Enabled = True
        btnSettingRFTX.Enabled = True
        chkbx_RFRX.Enabled = True
        btnSettingRFRX.Enabled = True

        chkbxRFTXSweep.Enabled = True
        btnSettingRFTXsweep.Enabled = True
        chkbxRFRXSweep.Enabled = True
        btnSettingRFRXsweep.Enabled = True

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

        gtxTest.txpowermode = spiTXTestSettings.eTxPwrMode.HIGH_TX_POWER_MODE

        gtxTest.descLen = spiTXTestSettings.HYBRII_CELLBUF_SIZE

        gtxTest.secTestMode = spiTXTestSettings.eSecTestMode.UNENCRYPTED
        gtxTest.eks = 15

        gtxTest.frmType = spiTXTestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU

        gtxTest.lenTestMode = spiTXTestSettings.eLenTestMode.FIXED_LEN
        gtxTest.mcstMode = spiTXTestSettings.eFrmMcstMode.HPGP_MCST

        '   Can be used for sweep test
        gtxTest.frmLen = 100
        gtxTest.delay = 10
        gtxTest.numFrames = 1000

    End Sub

    '
    '   Configure PLC sweep parameters
    '
    Private Sub sweepTest()
        varyFrmLen()
        swpCount = 1

        '   change other params
        For Each item As spiTXTestSettings._sPlcSimTxTestParams In sweepParamList
            item.frmType = spiTXTestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU
            item.plid = spiTXTestSettings.eHpgpPlidValue.HPGP_PLID0
            item.secTestMode = spiTXTestSettings.eSecTestMode.ENCRYPTED
            item.eks = 0
            item.delay = 4
            If item.frmLen = 100 Then
                item.ermode = 1
            End If
        Next

    End Sub

    '
    '   Vary frame length
    '
    Private Sub varyFrmLen()

        Dim temp_gtxtest As New spiTXTestSettings._sPlcSimTxTestParams
        temp_gtxtest = gtxTest

        For Each l As UInteger In FrmLenArr
            temp_gtxtest.frmLen = l
            If setPowerMode = False Then
                temp_gtxtest.txpowermode = spiTXTestSettings.eTxPwrMode.AUTOMOTIVE_TX_POWER_MODE
            End If
            sweepParamList.Enqueue(temp_gtxtest)
        Next

    End Sub

    '
    '   Configure RF sweep parameters
    '
    Private Sub RFChannelSweep()

        varyRFChannel()
        rfswpCount = 1
        swpCount = 1

    End Sub

    '
    '   Vary channel for RF sweep parameters
    '
    Private Sub varyRFChannel()

        Dim temp As New spiTXTestSettings.sRfTxTestParams
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
        rfgtxTest.interFrameDelay = 50

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
                lblSessionName.Text = chkbx_rxSweep.Text
                Exit Select

            Case tests.TEST_ID_PLC_TX
                lblSessionName.Text = chkbxPLCTX.Text
                Exit Select

            Case tests.TEST_ID_PLC_TX_SWEEP
                lblSessionName.Text = chkbx_txSweep.Text
                Exit Select

            Case tests.TEST_ID_RF_RX
                lblSessionName.Text = chkbx_RFRX.Text
                Exit Select

            Case tests.TEST_ID_RF_TX
                lblSessionName.Text = chkbx_RFTX.Text
                Exit Select

            Case tests.TEST_ID_RF_RX_SWEEP
                lblSessionName.Text = chkbxRFRXSweep.Text
                Exit Select

            Case tests.TEST_ID_RF_TX_SWEEP
                lblSessionName.Text = chkbxRFTXSweep.Text
                Exit Select

        End Select
    End Sub

    '
    '   Run the test
    '
    Private Sub btnRunTest_Click(sender As Object, e As EventArgs) Handles btnRunTest.Click

        s = New summary
        s.tests = New List(Of variations)

        txtbxDummy.Clear()

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
                            rfgtxTest.ch = &HF
                        End If
                        swap_dest_short_address()
                        RunTest.rftestParams = Me.rfgtxTest
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
                            rfgtxTest.ch = &HF
                        End If
                        RunTest.rftestParams = Me.rfgtxTest
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

                    'chkbxPLCTX.Checked = False
                    'chkbxPLCRX.Checked = False

                    '' To keep sweep tests running continuously
                    'chkbx_txSweep.Checked = False
                    'chkbx_txSweep.Checked = True
                    'chkbx_rxSweep.Checked = False
                    'chkbx_rxSweep.Checked = True

                    '' RF
                    'chkbx_RFTX.Checked = False
                    'chkbx_RFTX.Checked = True
                    'chkbx_RFRX.Checked = False
                    'chkbx_RFRX.Checked = True

                    enableRFTXSweep()
                    enableRFRXSweep()
                    enablePLCTXSweep()
                    enablePLCRXSweep()

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
                            rfgtxTest.ch = &HF
                        End If
                        RunTest.rftestParams = Me.rfgtxTest
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
                            rfgtxTest.ch = &HF
                        End If
                        swap_dest_short_address()
                        RunTest.rftestParams = Me.rfgtxTest
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

                    'chkbxPLCTX.Checked = False
                    'chkbxPLCRX.Checked = False

                    '' To keep sweep tests running continuously
                    'chkbx_txSweep.Checked = False
                    'chkbx_txSweep.Checked = True
                    'chkbx_rxSweep.Checked = False
                    'chkbx_rxSweep.Checked = True

                    '' RF
                    'chkbx_RFTX.Checked = False
                    'chkbx_RFTX.Checked = True
                    'chkbx_RFRX.Checked = False
                    'chkbx_RFRX.Checked = True

                    enableRFTXSweep()
                    enableRFRXSweep()
                    enablePLCTXSweep()
                    enablePLCRXSweep()

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
                        RunTest.gtxTest = Me.gtxTest
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
                        RunTest.gtxTest = Me.gtxTest
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

                    'chkbxPLCTX.Checked = False
                    'chkbxPLCRX.Checked = False

                    '' To keep sweep tests running continuously
                    'chkbx_txSweep.Checked = False
                    'chkbx_txSweep.Checked = True
                    'chkbx_rxSweep.Checked = False
                    'chkbx_rxSweep.Checked = True

                    '' RF
                    'chkbx_RFTX.Checked = False
                    'chkbx_RFTX.Checked = True
                    'chkbx_RFRX.Checked = False
                    'chkbx_RFRX.Checked = True

                    enableRFTXSweep()
                    enableRFRXSweep()
                    enablePLCTXSweep()
                    enablePLCRXSweep()

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

                    'chkbxPLCTX.Checked = False
                    'chkbxPLCRX.Checked = False

                    '' To keep sweep tests running continuously
                    'chkbx_txSweep.Checked = False
                    'chkbx_txSweep.Checked = True
                    'chkbx_rxSweep.Checked = False
                    'chkbx_rxSweep.Checked = True

                    '' RF
                    'chkbx_RFTX.Checked = False
                    'chkbx_RFTX.Checked = True
                    'chkbx_RFRX.Checked = False
                    'chkbx_RFRX.Checked = True

                    enableRFTXSweep()
                    enableRFRXSweep()
                    enablePLCTXSweep()
                    enablePLCRXSweep()

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

        gtxTest = sweepParamList.First
        sweepTestRunning = True

        If setPowerMode = True Then
            dumpMsg("High Power Mode")
            dumpMsg("-------------------------")    '25 Dashes
        Else
            dumpMsg("Low Power Mode")
            dumpMsg("-------------------------")    '25 Dashes
        End If

        status.test = tests.TEST_ID_PLC_TX
        status.state = RunTest.states.STATE_PREPARE_REFERENCE

        _test_plcTX(status)

    End Sub

    '   <summary>
    '               PLC RX SWEEP Sub
    '   </summary>
    Private Sub _test_plcRXSweep(ByVal status As execState)

        sweepTest()

        status.test = tests.TEST_ID_PLC_RX
        status.state = RunTest.states.STATE_PREPARE_DUT

        gtxTest = sweepParamList.First
        sweepTestRunning = True

        If setPowerMode = True Then
            dumpMsg("High Power Mode")
            dumpMsg("-------------------------")    '25 Dashesh
        Else
            dumpMsg("Low Power Mode")
            dumpMsg("-------------------------")    '25 Dashesh
        End If

        _test_plcRX(status)

    End Sub

#End Region

    '   <summary>
    '           Checkbox and settings button events
    '   </summary>
#Region "Wrapper for button events"

#Region "Enable"
    '
    '   Enable RF TX
    '
    Private Sub enableRFTX()
        chkbx_RFTX.Checked = False
        chkbx_RFTX.Checked = True
    End Sub

    '
    '   Enable RF TX SWEEP
    '
    Private Sub enableRFTXSweep()
        chkbxRFTXSweep.Checked = False
        chkbxRFTXSweep.Checked = True
    End Sub

    '
    '   Enable RF RX
    '
    Private Sub enableRFRX()
        chkbx_RFRX.Checked = False
        chkbx_RFRX.Checked = True
    End Sub

    '
    '   Enable RF RX SWEEP
    '
    Private Sub enableRFRXSweep()
        chkbxRFRXSweep.Checked = False
        chkbxRFRXSweep.Checked = True
    End Sub

    '
    '   Enable PLC TX
    '
    Private Sub enablePLCTX()
        chkbxPLCTX.Checked = False
        chkbxPLCTX.Checked = True
    End Sub

    '
    '   Enable PLC TX SWEEP
    '
    Private Sub enablePLCTXSweep()
        chkbx_txSweep.Checked = False
        chkbx_txSweep.Checked = True
    End Sub

    '
    '   Enable PLC RX
    '
    Private Sub enablePLCRX()
        chkbxPLCRX.Checked = False
        chkbxPLCRX.Checked = True
    End Sub

    '
    '   Enable PLC TX SWEEP
    '
    Private Sub enablePLCRXSweep()
        chkbx_rxSweep.Checked = False
        chkbx_rxSweep.Checked = True
    End Sub

#End Region

#Region "Disable"
    '
    '   Disable RF TX
    '
    Private Sub disableRFTX()

    End Sub

    '
    '   Disable RF TX SWEEP
    '
    Private Sub disableRFTXSweep()

    End Sub

    '
    '   Disable RF RX
    '
    Private Sub disableRFRX()

    End Sub

    '
    '   Disable RF RX SWEEP
    '
    Private Sub disableRFRXSweep()

    End Sub

    '
    '   Disable PLC TX
    '
    Private Sub disablePLCTX()

    End Sub

    '
    '   Disable PLC TX SWEEP
    '
    Private Sub disablePLCTXSweep()

    End Sub

    '
    '   Disable PLC RX
    '
    Private Sub disablePLCRX()

    End Sub

    '
    '   Disable PLC TX SWEEP
    '
    Private Sub disablePLCRXSweep()

    End Sub

#End Region
    
#End Region

    '   <summary>
    '           Checkbox and settings button events
    '   </summary>
#Region "Checkbox and Settings Button Events"
    '
    '   PLC TX Sweep Test checkbox
    '
    Private Sub chkbx_txSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_txSweep.CheckedChanged

        If chkbx_txSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_PLC_TX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_PLC_TX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX))
                    chkbxPLCTX.Checked = False
                    btn_txSweepSettings.Enabled = False
                End If
            End If
            btn_txSweepSettings.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX_SWEEP))
            End If
        End If
    End Sub
    '
    '   PLC RX Sweep Test checkbox
    '
    Private Sub chkbx_rxSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_rxSweep.CheckedChanged

        If chkbx_rxSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_PLC_RX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX))
                    chkbxPLCRX.Checked = False
                    btn_rxSweepSettings.Enabled = False
                End If
            End If
            btn_rxSweepSettings.Enabled = True
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
            btnSettingsPLCTX.Enabled = True
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
            btnSettingsPLCRX.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX))
            End If
        End If
    End Sub

    '
    '   RF TX Test enque
    '
    Private Sub chkbx_RFTX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_RFTX.CheckedChanged
        '   Set status test
        If chkbx_RFTX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_TX) Or qExecStatus.Contains(tests.TEST_ID_RF_TX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_TX)
            End If
            btnSettingRFTX.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_TX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_TX))
            End If
        End If
    End Sub

    '
    '   RF RX Test enque
    '
    Private Sub chkbx_RFRX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_RFRX.CheckedChanged
        '   Set status test
        If chkbx_RFRX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_RX) Or qExecStatus.Contains(tests.TEST_ID_RF_RX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_RX)
            End If
            btnSettingRFRX.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_RX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_RX))
            End If
        End If

    End Sub

    '
    '   RF TX Sweep enque
    '
    Private Sub chkbxRFTXSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxRFTXSweep.CheckedChanged

        If chkbxRFTXSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_TX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_TX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_RF_TX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_TX))
                    chkbx_RFTX.Checked = False
                    btnSettingRFTX.Enabled = False
                End If
            End If
            btnSettingRFTXsweep.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_TX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_TX_SWEEP))
            End If
        End If

    End Sub

    '
    '   RF RX Sweep enque
    '
    Private Sub chkbxRFRXSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxRFRXSweep.CheckedChanged

        If chkbxRFRXSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_RF_RX_SWEEP) Then
                ' do nothing cause the test is already enqued
            Else
                qExecStatus.Add(tests.TEST_ID_RF_RX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_RF_RX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_RX))
                    chkbx_RFRX.Checked = False
                    btnSettingRFRX.Enabled = False
                End If
            End If
            btnSettingRFRXsweep.Enabled = True
        Else
            If qExecStatus.Contains(tests.TEST_ID_RF_RX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_RF_RX_SWEEP))
            End If
        End If

    End Sub

    '
    '   View Transmit Test settings
    '
    Private Sub btnSettingsSPITX_Click(sender As Object, e As EventArgs) Handles btnSettingsPLCTX.Click
        testParamsFor = tests.TEST_ID_PLC_TX
        spiTXTestSettings.Show()
    End Sub

    Private Sub btnSettingsSPIRX_Click(sender As Object, e As EventArgs) Handles btnSettingsPLCRX.Click
        testParamsFor = tests.TEST_ID_PLC_RX
        spiTXTestSettings.Show()
    End Sub

    Private Sub btn_txSweepSettings_Click(sender As Object, e As EventArgs) Handles btn_txSweepSettings.Click
        testParamsFor = tests.TEST_ID_PLC_TX_SWEEP
        spiTXTestSettings.Show()
    End Sub

    Private Sub btn_rxSweepSettings_Click(sender As Object, e As EventArgs) Handles btn_rxSweepSettings.Click
        testParamsFor = tests.TEST_ID_PLC_RX_SWEEP
        spiTXTestSettings.Show()
    End Sub

#End Region

    '   <summary>
    '           Result log browsing
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

    '
    '   Reset Devices
    '
    Private Sub btn_ResetDevices_Click(sender As Object, e As EventArgs) Handles btn_ResetDevices.Click
        For Each cl As ConnectedClient In clients
            RunTest.beginSend(RunTest.states.STATE_DEVICE_RESET, cl, Nothing)
        Next
    End Sub

    '   <summary>
    '           Changing local machine IPv4 addres
    '   </summary>
#Region "Configure Local Machine IPv4 Address"

    '
    '   Get IP Address of local machine
    '
    Function GetIP() As String

        Return (
            From networkInterface In networkInterface.GetAllNetworkInterfaces()
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
    '           These are utility functions used in tool
    '   </summary>
#Region "Utility Functions"

    '   <summary>
    '           Utility functions for progress bar
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
    '           Convert byte array to structure
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
    '           Convert byte array to structure
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

    '
    '   Dump messages on dummy text box
    '
    Private objForUI = New Object()
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


End Class
