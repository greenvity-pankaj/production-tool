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
    '       |       |       |       |RX_SWEEP|TX_SWEEP| PLC RX| PLC TX|

    Enum bitmap
        PLC_TX
        PLC_RX
        TX_SWEEP
        RX_SWEEP
        SPI_TX
        SPI_RX
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
        Public testPramas As spiTXTestSettings._sPlcSimTxTestParams
        Public rsp As shpgpStats._plcTxTestResults_t
    End Structure
#End Region

    '   <summary>
    '       Declaration of variables used in the program
    '   </summary>
#Region "Required Variables"

    '   Boolean Variables
    Public DUT_UP As Boolean = False
    Public REF_UP As Boolean = False
    Public lineSent As Boolean = False
    Public devicesUP As Boolean = False
    Public testRunning As Boolean = False
    Public setPowerMode As Boolean = True
    Private sweepTestRunning As New Boolean
    Public userPLCTXPramasSet As New Boolean
    Public userPLCRXPramasSet As New Boolean

    '   Delegates
    Private Delegate Sub _initUIDelegate()
    Private Delegate Function getSelectedLVIItemDelegate()
    Private Delegate Sub _AddClient(ByVal client As Socket, ByVal type As RunTest.ClientType)

    '   Directory
    Public clDirectory = New Dictionary(Of String, ConnectedClient)

    '   Enum Variables
    Public testParamsFor As tests

    '   Duration Calculations
    Private stop_time As DateTime = Nothing
    Private start_time As DateTime = Nothing
    Private elapsed_time As TimeSpan = Nothing

    '   Integers Variables
    Dim minVal As Decimal = 0
    Dim maxVal As Decimal = 130
    Private port As Integer = 54321
    Private swpCount As New UInteger
    Private runCount As New UInteger
    Private threshold As UInteger = 10
    Private serverThreadCount As Integer = 0
    Private pendingConnections As Integer = 5

    '   Lists
    Public sockets As List(Of Socket)
    Public clients As New List(Of ConnectedClient)
    Public qExecStatus As New List(Of tests)
    Public defTestQ As New List(Of tests)
    Private FrmLenArr As New List(Of UInteger)(New UInteger() {100, 250, 500, 800, 1000})
    'Private FrmLenArr As New List(Of UInteger)(New UInteger() {100, 250})

    '   Queue
    Private sweepParamList As New Queue(Of spiTXTestSettings._sPlcSimTxTestParams)

    '   Sockets
    Public serverSocket As Socket
    Public clientSocket As Socket
    Private serverStream As NetworkStream
    Private listener As System.Net.Sockets.TcpListener

    '   String Variables
    Private st As String
    Private filereader As String

    '   Structure Variables
    Public m As metadata
    Public status As New execState
    Public plcRXparams As New spiTXTestSettings._sPlcSimTxTestParams
    Public plcTXparams As New spiTXTestSettings._sPlcSimTxTestParams
    Public gtxTest As New spiTXTestSettings._sPlcSimTxTestParams  'global

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
        TEST_ID_SPI_TX
        TEST_ID_SPI_RX
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
        FAILURE
        INVALID
    End Enum

#End Region


    Private Sub HomeScreen_Disposed(sender As Object, e As EventArgs) Handles Me.Disposed
        Try
            For Each cl As ConnectedClient In clients
                cl.mClient.Client.Shutdown(SocketShutdown.Both)
            Next
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try

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
        'RunThreads(serverThreadCount, AddressOf startServer)

        '   enqueue tests and configure default params
        setDefaultPramas()
        defTestQ.Add(tests.TEST_ID_PLC_TX_SWEEP)
        defTestQ.Add(tests.TEST_ID_PLC_RX_SWEEP)

        Try
            m.filePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "Test Results.txt")
            If Not System.IO.File.Exists(m.filePath) Then
                System.IO.File.Create(m.filePath).Dispose()
            End If
        Catch fileException As Exception
            Throw fileException
        End Try

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
            btnSettingsPLCRX.Enabled = False
            btnSettingsPLCTX.Enabled = False
            btn_ResetDevices.Enabled = False

            '   Test Check boxes
            Checkbox.Enabled = False
            chkbxPLCRX.Enabled = False
            chkbxPLCTX.Enabled = False

            '   Set bar thresholds
            bar.Maximum = maxVal
            bar.Minimum = minVal
            bar.Step = 10
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
            Throw ex
        End Try

    End Sub

    '
    '   do listen
    '
    Private Sub doListen()

        Do
            Dim incomingClient As New System.Net.Sockets.TcpClient
            incomingClient = listener.AcceptTcpClient
            Dim connClient As New ConnectedClient(incomingClient)
            AddHandler connClient.dataReceived, AddressOf Me.messageReceived
            clients.Add(connClient)
            lvClients.Items.Add(incomingClient.Client.RemoteEndPoint.ToString.Split(":"c).First)
        Loop

    End Sub

    '
    '   Remove client
    '
    Public Sub removeClient(ByVal client As ConnectedClient)

        If clients.Contains(client) Then

            clients.Remove(client)
            dumpMsg("Client removed !!")
        End If

    End Sub

    '
    '   Scan Devices
    '
    Private Sub btn_ScanDevices_Click_1(sender As Object, e As EventArgs) Handles btn_ScanDevices.Click

        If clients.Count > 0 Then

            If lvClients.Items.Count > 0 Then
                lvClients.Items.Clear()
                txtbxDummy.Clear()
            End If

            For Each cl As ConnectedClient In clients
                RunTest.beginSend(RunTest.states.STATE_DEVICE_SEARCH, cl, Nothing)
            Next

        Else
            MessageBox.Show("No clients Detected !")
            MessageBox.Show("Make sure ARM9 clients are up !")
            Exit Sub
        End If

    End Sub

    '
    '   check if both devices are up
    '
    Private Function isDeviceUP(ByVal sender As ConnectedClient, ByVal deviceUPEvent As RunTest.sDevUpEvent, _
                                ByRef DUT_UP As Boolean, ByRef REF_UP As Boolean, ByRef devicesUP As Boolean) As Boolean

        If deviceUPEvent.deviceType = RunTest.ClientType.REF Then
            sender.mDevType = deviceUPEvent.deviceType

            clDirectory(sender.mClient.Client.RemoteEndPoint.ToString.Split(":"c).First) = sender

            AddClient(sender.mClient.Client, RunTest.ClientType.REF)
            REF_UP = True
        Else
            If deviceUPEvent.deviceType = RunTest.ClientType.DUT Then
                sender.mDevType = deviceUPEvent.deviceType

                clDirectory(sender.mClient.Client.RemoteEndPoint.ToString.Split(":"c).First) = sender

                AddClient(sender.mClient.Client, RunTest.ClientType.DUT)
                DUT_UP = True
            End If
        End If

        If DUT_UP = True And REF_UP = True Then
            devicesUP = True

            '   check boxes for default tests
            chkbx_txSweep.Checked = True
            chkbx_rxSweep.Checked = True
            btn_rxSweepSettings.Enabled = True
            btn_txSweepSettings.Enabled = True

            dumpMsg("Tests are configured ! ")
            dumpMsg("Select the devices and then You can start the tests ! ")
            dumpMsg(vbNewLine)

            btnRunTest.Enabled = True

            Return devicesUP
        End If

        Return False

    End Function
    '
    '   Add client (invoke by Delegate)
    '
    Public Sub AddClient(ByVal client As Socket, ByVal type As RunTest.ClientType)

        Dim clIP As String() = client.RemoteEndPoint.ToString.Split(":"c)
        Dim lvi As New ListViewItem(clIP.First & " " & type.ToString)
        lvi.Text = clIP.First & " " & type.ToString
        lvi.Tag = client
        lvClients.Items.Add(lvi.Text)

    End Sub

    '
    '   to get the selected client
    '
    Public Function getSelectedClientList() As List(Of ConnectedClient)

        Dim list = New List(Of ConnectedClient)

        For Each s As String In lvClients.SelectedItems

            Dim cl As ConnectedClient
            cl = clDirectory(s.Split(" "c).First)
            list.Add(cl)

        Next

        Return list
    End Function

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
                        chkbxPLCTX.Enabled = True
                        Exit Select
                    Case bitmap.PLC_RX
                        chkbxPLCRX.Enabled = True
                        Exit Select
                    Case bitmap.TX_SWEEP
                        chkbx_txSweep.Enabled = True
                        Exit Select
                    Case bitmap.RX_SWEEP
                        chkbx_rxSweep.Enabled = True
                        Exit Select
                        'Case bitmap.SPI_TX
                        '    Checkbox.Enabled = True
                        '    Exit Select
                        'Case bitmap.SPI_RX
                        '    Checkbox.Enabled = True
                        '    Exit Select
                End Select
                bitNum -= 1
            Else
                bitNum -= 1
            End If
        Next
    End Sub

    '   <summary>
    '       State machine to handle the received message
    '   </summary>
#Region "Received Message Handler"
    '
    '   message received
    '
    Private Sub messageReceived(ByVal sender As ConnectedClient, ByVal packet As Byte())


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

                            If isDeviceUP(sender, deviceUPEvent, REF_UP, DUT_UP, devicesUP) = False Then
                                If DUT_UP = True Then
                                    enableTests(deviceUPEvent.bMapTests)
                                Else
                                    If REF_UP = True Then
                                        enableTests(deviceUPEvent.bMapTests)
                                    End If
                                End If
                                Exit Sub
                            End If
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
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_STOP_TEST
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_STOP_TEST
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_STOP_TEST
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_STOP_TEST
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                End If

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
                            '   sort the response
                            Dim resp As New RunTest.sResponse
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray, _
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            If resp.rsp = response.SUCCESS Then
                                updateBar()
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_START_TEST_REF
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_PREPARE_REFERENCE
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_START_TEST_REF
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_PREPARE_REFERENCE
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                End If

                            End If
                            Exit Select

                        Case commandIDs.TOOL_CMD_PREPARE_REFERENCE_CNF
                            '   sort the response
                            Dim resp As New RunTest.sResponse
                            Array.Copy(packet, Marshal.SizeOf(GetType(RunTest.frmHeader)), respArray, _
                                       0, Marshal.SizeOf(GetType(RunTest.sResponse)))

                            resp = CType(ByteToStruct(respArray, GetType(RunTest.sResponse)), RunTest.sResponse)

                            If resp.rsp = response.SUCCESS Then
                                updateBar()
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_PREPARE_DUT
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_START_TEST_DUT
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_PREPARE_DUT
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_START_TEST_DUT
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

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
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Then

                                    If sender.mDevType = RunTest.ClientType.REF Then
                                        status.test = tests.TEST_ID_PLC_TX
                                        status.state = RunTest.states.STATE_START_TEST_DUT
                                        'executeTests(status)
                                    End If

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then

                                    If sender.mDevType = RunTest.ClientType.DUT Then
                                        status.test = tests.TEST_ID_PLC_RX
                                        status.state = RunTest.states.STATE_START_TEST_REF
                                        'executeTests(status)
                                    End If

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    If sender.mDevType = RunTest.ClientType.REF Then
                                        status.test = tests.TEST_ID_PLC_TX
                                        status.state = RunTest.states.STATE_START_TEST_DUT
                                        '   Since for PLC TX test REF gives stop test confirm
                                        'executeTests(status)
                                    End If

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    If sender.mDevType = RunTest.ClientType.DUT Then
                                        status.test = tests.TEST_ID_PLC_RX
                                        status.state = RunTest.states.STATE_START_TEST_REF
                                        '   Since for PLC TX test REF gives stop test confirm
                                        'executeTests(status)
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
                                If qExecStatus.First = tests.TEST_ID_PLC_TX Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_GET_RESULT
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_GET_RESULT
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_GET_RESULT
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_GET_RESULT
                                    '   Since for PLC TX test REF gives stop test confirm
                                    'executeTests(status)

                                End If
                            End If
                            Exit Select

                        Case commandIDs.TOOL_CMD_GET_RESULT_CNF
                            '   sort the response

                            Dim statsArray As New List(Of Byte)
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
                            UpdateDuration()
                            updatemetadata()
                            decideTestStatus()

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
                                End If
                                'enableUI()
                                clearBar()
                                'executeTests(status)

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
                                End If
                                'enableUI()
                                clearBar()
                                'executeTests(status)

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
                                        'executeTests(status)
                                        Exit Select

                                    End If

                                    qExecStatus.RemoveAt(0)

                                    chkbx_txSweep.Checked = False
                                    chkbxPLCRX.Checked = False
                                    chkbxPLCTX.Checked = False

                                    setDefaultPramas()
                                    sweepParamList.Clear()
                                    swpCount = 0

                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing

                                    testRunning = False
                                    If qExecStatus.Count = 0 Then
                                        lineSent = False
                                        enableUI()
                                    End If
                                    sweepTestRunning = False
                                    setPowerMode = True

                                    'enableUI()
                                    clearBar()
                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_GET_RESULT_CNF
                                Else
                                    status.test = tests.TEST_ID_PLC_TX
                                    status.state = RunTest.states.STATE_PREPARE_REFERENCE

                                    gtxTest = sweepParamList.Peek
                                    start_time = Now
                                End If

                                'executeTests(status)

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
                                        'executeTests(status)
                                        Exit Select

                                    End If

                                    qExecStatus.RemoveAt(0)

                                    chkbx_rxSweep.Checked = False
                                    chkbxPLCRX.Checked = False
                                    chkbxPLCTX.Checked = False

                                    testRunning = False
                                    If qExecStatus.Count = 0 Then
                                        lineSent = False
                                        enableUI()
                                    End If
                                    sweepTestRunning = False
                                    setPowerMode = True

                                    setDefaultPramas()
                                    sweepParamList.Clear()
                                    swpCount = 0

                                    start_time = Nothing
                                    stop_time = Nothing
                                    elapsed_time = Nothing

                                    'enableUI()
                                    clearBar()
                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_GET_RESULT_CNF
                                Else
                                    status.test = tests.TEST_ID_PLC_RX
                                    status.state = RunTest.states.STATE_PREPARE_DUT

                                    gtxTest = sweepParamList.Peek
                                    start_time = Now
                                End If

                                'executeTests(status)

                            End If

                            Exit Select
                    End Select
                    '   Response Switch Ends
                    Exit Select
            End Select
            executeTests(status)
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

        f = (((m.testPramas.numFrames - m.rsp.TotalRxGoodFrmCnt) / m.testPramas.numFrames) * 100)

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

        If m.testStatus = True Then
            dumpMsg(m.name & vbNewLine & "Frame Length - " & m.testPramas.frmLen)
            dumpMsg("Status" & " : " & "PASS" & vbNewLine)
        ElseIf m.testStatus = False Then
            dumpMsg(m.name & vbNewLine & "Frame Length - " & m.testPramas.frmLen)
            dumpMsg("Status" & " : " & "FAIL" & vbNewLine)
        End If

    End Sub

    '
    '   Update metadata
    '
    Private Sub updatemetadata()

        m.startTime = start_time
        m.stopTime = stop_time
        m.duration = elapsed_time.ToString
        m.testPramas = gtxTest
        m.runCount = runCount
        m.threshold = threshold

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
        End If

    End Sub

    '
    '   Disable UI functions while running tests
    '
    Private Sub disableUI()

        btn_ScanDevices.Enabled = False

        btn_txSweepSettings.Enabled = False
        btn_rxSweepSettings.Enabled = False
        btnSettingsPLCRX.Enabled = False
        btnSettingsPLCTX.Enabled = False

        btnRunTest.Enabled = False
        btn_ShowResults.Enabled = False
        btn_SetIP.Enabled = False

        chkbx_rxSweep.Enabled = False
        chkbx_txSweep.Enabled = False
        chkbxPLCRX.Enabled = False
        chkbxPLCTX.Enabled = False

    End Sub

    '
    '   Enable UI functions disabled earlier during the test
    '
    Private Sub enableUI()

        btn_ScanDevices.Enabled = True

        btn_txSweepSettings.Enabled = True
        btn_rxSweepSettings.Enabled = True
        btnSettingsPLCRX.Enabled = True
        btnSettingsPLCTX.Enabled = True

        btnRunTest.Enabled = True
        btn_ShowResults.Enabled = True
        btn_SetIP.Enabled = True

        chkbx_rxSweep.Enabled = True
        chkbx_txSweep.Enabled = True
        chkbxPLCRX.Enabled = True
        chkbxPLCTX.Enabled = True

    End Sub

    '   <summary>
    '           State machines for sweep tests
    '   </summary>
#Region "Sweep Test"

    Private Sub sweepTest()
        varyFrmLen()
        swpCount = 1

        '   change other params
        For Each item As spiTXTestSettings._sPlcSimTxTestParams In sweepParamList
            item.frmType = spiTXTestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU
            item.plid = 0
            item.secTestMode = spiTXTestSettings.eSecTestMode.ENCRYPTED
            item.eks = 0
            item.delay = 4
        Next
        
    End Sub

    Private Sub varyFrmLen()

        Dim temp_gtxtest As New spiTXTestSettings._sPlcSimTxTestParams
        temp_gtxtest = gtxTest

        For Each l As UInteger In FrmLenArr
            temp_gtxtest.frmLen = l
            If setPowerMode = False Then
                temp_gtxtest.txpowermode = 0
            End If
            sweepParamList.Enqueue(temp_gtxtest)
        Next

    End Sub

#End Region

    '   <summary>
    '           State machines for individual tests
    '   </summary>
#Region "Execution Subs"
    '
    '   Run the test
    '
    Private Sub btnRunTest_Click(sender As Object, e As EventArgs) Handles btnRunTest.Click

        If getSelectedClientList().Count <= 1 Then
            MessageBox.Show("Select more than one devices and Restart the test !!")
            Exit Sub
        End If

        If qExecStatus.Count = 0 Then
            MessageBox.Show("Select atleast one test !!")
            Exit Sub
        End If

        runCount += 1
        testRunning = True
        disableUI()


        If qExecStatus.First = tests.TEST_ID_PLC_TX Then

            status.test = tests.TEST_ID_PLC_TX
            status.state = RunTest.states.STATE_PREPARE_REFERENCE

        ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX Then

            status.test = tests.TEST_ID_PLC_RX
            status.state = RunTest.states.STATE_PREPARE_DUT

        ElseIf qExecStatus.First = tests.TEST_ID_PLC_TX_SWEEP Then

            status.test = tests.TEST_ID_PLC_TX_SWEEP
            status.state = RunTest.states.STATE_PREPARE_REFERENCE

        ElseIf qExecStatus.First = tests.TEST_ID_PLC_RX_SWEEP Then

            status.test = tests.TEST_ID_PLC_RX_SWEEP
            status.state = RunTest.states.STATE_PREPARE_DUT

        End If

        start_time = Now
        executeTests(status)

    End Sub
    '
    '   Sequenced tests
    '
    Public Sub executeTests(ByVal status As HomeScreen.execState)

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

        End Select

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

                    End If

                    start_time = Now
                    executeTests(status)

                Else

                    chkbxPLCTX.Checked = False
                    chkbxPLCRX.Checked = False

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

                    End If

                    start_time = Now
                    executeTests(status)

                Else

                    chkbxPLCTX.Checked = False
                    chkbxPLCRX.Checked = False

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

        status.test = tests.TEST_ID_PLC_TX
        status.state = RunTest.states.STATE_PREPARE_REFERENCE

        gtxTest = sweepParamList.First
        sweepTestRunning = True

        If setPowerMode = True Then
            dumpMsg("High Power Mode")
            dumpMsg("-------------------------" & vbNewLine)    '25 Dashesh
        Else
            dumpMsg("Low Power Mode")
            dumpMsg("-------------------------" & vbNewLine)    '25 Dashesh
        End If

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
            dumpMsg("-------------------------" & vbNewLine)    '25 Dashesh
        Else
            dumpMsg("Low Power Mode")
            dumpMsg("-------------------------" & vbNewLine)    '25 Dashesh
        End If

        _test_plcRX(status)

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
    '   Msg print
    '
    Public Sub dumpMsg(ByVal msg As String)
        With txtbxDummy
            .AppendText(msg)
            .AppendText(vbNewLine)
            .ScrollToCaret()
        End With
    End Sub

 

#Region "Checkbox and Settings Button Events"
    '
    '   TX Sweep Test checkbox
    '
    Private Sub chkbx_txSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_txSweep.CheckedChanged

        If chkbx_txSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then

            Else
                qExecStatus.Add(tests.TEST_ID_PLC_TX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_PLC_TX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX))
                    chkbxPLCTX.Checked = False
                    btn_txSweepSettings.Enabled = False
                End If
            End If
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX_SWEEP))
            End If
        End If

        lblSessionName.Text = chkbx_txSweep.Text
        btn_txSweepSettings.Enabled = True
        btnRunTest.Enabled = True

    End Sub
    '
    '   RX Sweep Test checkbox
    '
    Private Sub chkbx_rxSweep_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_rxSweep.CheckedChanged

        If chkbx_rxSweep.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX_SWEEP) Then

            Else
                qExecStatus.Add(tests.TEST_ID_PLC_RX_SWEEP)
                If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Then
                    qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX))
                    chkbxPLCRX.Checked = False
                    btn_rxSweepSettings.Enabled = False
                End If
            End If
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX_SWEEP) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX_SWEEP))
            End If
        End If

        lblSessionName.Text = chkbx_rxSweep.Text
        btn_rxSweepSettings.Enabled = True
        btnRunTest.Enabled = True

    End Sub
    '
    '   PLC TX Test parameter selection
    '
    Private Sub chkbxSPITX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxPLCTX.CheckedChanged
        '   Set status test
        If chkbxPLCTX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX) Or qExecStatus.Contains(tests.TEST_ID_PLC_TX_SWEEP) Then

            Else
                qExecStatus.Add(tests.TEST_ID_PLC_TX)
            End If
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_TX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_TX))
            End If
        End If

        lblSessionName.Text = chkbxPLCTX.Text
        btnSettingsPLCTX.Enabled = True
        btnRunTest.Enabled = True

    End Sub
    '
    '   SPI RX Test parameter selection
    '
    Private Sub chkbxSPIRX_CheckedChanged(sender As Object, e As EventArgs) Handles chkbxPLCRX.CheckedChanged
        '   Set status test
        If chkbxPLCRX.Checked = True Then
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Or qExecStatus.Contains(tests.TEST_ID_PLC_RX_SWEEP) Then

            Else
                qExecStatus.Add(tests.TEST_ID_PLC_RX)
            End If
        Else
            If qExecStatus.Contains(tests.TEST_ID_PLC_RX) Then
                qExecStatus.RemoveAt(qExecStatus.IndexOf(tests.TEST_ID_PLC_RX))
            End If
        End If

        lblSessionName.Text = chkbxPLCRX.Text
        btnSettingsPLCRX.Enabled = True
        btnRunTest.Enabled = True
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

#Region "Result log location browsing"

    '
    '   Seesion location browsing
    '
    Private Sub btnBrowseLocation_Click(sender As Object, e As EventArgs) Handles btnBrowseLocation.Click

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

    '
    '   Set the default parameters for the tests
    '
    Private Sub setDefaultPramas()

        gtxTest.snid = 1
        gtxTest.dtei = 2
        gtxTest.stei = 3

        gtxTest.txpowermode = 2

        gtxTest.descLen = spiTXTestSettings.HYBRII_CELLBUF_SIZE

        gtxTest.secTestMode = spiTXTestSettings.eSecTestMode.UNENCRYPTED
        gtxTest.eks = 15

        gtxTest.frmType = spiTXTestSettings.eFrmType.HPGP_HW_FRMTYPE_MSDU

        gtxTest.lenTestMode = spiTXTestSettings.eLenTestMode.FIXED_LEN
        gtxTest.mcstMode = &H1

        '   Can be used for sweep test
        gtxTest.frmLen = 100
        gtxTest.delay = 10
        gtxTest.numFrames = 1000

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
    '           Utility functions for progress bar
    '   </summary>
#Region "Progress Bar"

    Private Sub updateBar()

        bar.Step = 10
        bar.PerformStep()

        If bar.Value >= bar.Maximum Then
            System.Threading.Thread.Sleep(30)
            bar.Value = bar.Minimum
        End If

    End Sub

    Private Sub clearBar()

        System.Threading.Thread.Sleep(30)
        bar.Value = bar.Minimum

    End Sub

#End Region
    
    

End Class
