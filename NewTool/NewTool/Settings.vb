Imports System.Runtime.Serialization.Formatters.Binary
Imports System.Net, System.Net.Sockets, System.Text, System.Threading
Imports System.Runtime.InteropServices

Public Class spiTXTestSettings
    '   <summary>
    '       Enum declarations
    '   </summary>
#Region "Enum Declaration"

    '
    '   Security Modes
    '
    Enum eSecTestMode As Byte
        UNENCRYPTED = &H0
        ENCRYPTED = &H1             'TestParams.eks gives the key
        ALT_UNENC_NEK = &H2
        ALT_UNENC_NEK_PPEK = &H3    'alternating non_secure, eks1 (nek), eks2(ppek)
    End Enum

    '
    '   Frame transmit mode
    '
    Enum eFrmMcstMode As Byte
        HPGP_UCST = &H0
        HPGP_MCST = &H1
        HPGP_MNBCST = &H2
    End Enum

    '
    '   plid value
    '
    Enum eHpgpPlidValue As Byte
        HPGP_PLID0 = &H0
        HPGP_PLID1 = &H1
        HPGP_PLID2 = &H2
        HPGP_PLID3 = &H3
    End Enum

    '
    '   Packet Length
    '
    Enum eLenTestMode As Byte
        INC_LEN_SINGLE_ROBO = &H0   'Incremental length within any one robo mode better not mix this with alt Frm type
        INC_LEN_ALL_ROBO = &H1      'Incremental length spanning across all robo modes better not mix this with alt Frm type
        FIXED_LEN_ALT_ROBO = &H2    'One fixed length for each Robo Test Mode
        FIXED_LEN = &H3
        VARY_LEN = &H4
    End Enum

    '
    '   Robo Test Mode
    '
    Enum eRoboTestMode As Byte
        MINI_ROBO_TEST = &H0
        STD_ROBO_TEST = &H1
        HS1PB_ROBO_TEST = &H2
        HS2PB_ROBO_TEST = &H3
        HS3PB_ROBO_TEST = &H4
        HSALLPB_ROBO_TEST = &H5    'Combination of 1,2 and 3 PB HS
    End Enum

    '
    '   Frame type
    '
    Enum eFrmType As Byte
        HPGP_HW_FRMTYPE_SOUND = &H0
        HPGP_HW_FRMTYPE_MSDU = &H1
        HPGP_HW_FRMTYPE_BEACON = &H2
        HPGP_HW_FRMTYPE_MGMT = &H3
        HPGP_HW_FRMTYPE_RTS = &H4
        HPGP_HW_FRMTYPE_CTS = &H5
    End Enum

#End Region

    '   <summary>
    '       Structure Declarations for tests
    '   </summary>
#Region "Structure Declerations"

    <StructLayout(LayoutKind.Sequential, Size:=37, pack:=1)> _
    Public Structure _sPlcSimTxTestParams
        '   Security
        Public eks As Byte
        Public secTestMode As Byte

        'frame type
        Public frmType As Byte
        Public altFrmTypeTest As Byte           'overrides frmType

        'ucst/mcst/bcst
        Public mcstMode As Byte
        Public altMcstTest As Byte

        'plid
        Public plid As Byte
        Public altPlidTest As Byte              'overrides plid

        'offset descLen
        Public offsetDW As Byte
        Public descLen As Byte
        Public altOffsetDescLenTest As Byte

        'robo mode/ lengths
        Public stdModeSel As Byte               'for frm lens b/w Mini Robo Max & STD/2PB HS Max
        Public lenTestMode As Byte
        Public roboTestMode As Byte             'for single robo inc len test

        Public contMode As Byte
        Public frmLen As UShort
        Public numFrames As UInteger
        Public delay As UInteger
        Public dt_av As Byte
        Public src As Byte                      'Sound Reason Code
        Public saf As Byte
        Public scf As Byte
        Public plcMultiPktTest As Byte
        Public dbc As Byte
        Public pattern As Byte

        Public snid As Byte
        Public dtei As Byte
        Public stei As Byte

        Public txpowermode As Byte
        Public ermode As Byte
    End Structure

    Const MAC_ADD_LEN As Integer = 8

    '
    '   RF Parameters
    '
    <StructLayout(LayoutKind.Sequential, Pack:=1, size:=20)> _
    Public Structure sRfTxTestParams
        <FieldOffset(0)> <MarshalAs(UnmanagedType.ByValArray, SizeConst:=MAC_ADD_LEN)> Public macAddress As Byte()
        <FieldOffset(8)> <MarshalAsAttribute(UnmanagedType.U2)> Public macShortAddress As UShort
        <FieldOffset(10)> <MarshalAsAttribute(UnmanagedType.U2)> Public dstShortAddress As UShort
        <FieldOffset(12)> <MarshalAsAttribute(UnmanagedType.U1)> Public ch As Byte
        <FieldOffset(13)> <MarshalAsAttribute(UnmanagedType.U2)> Public panId As UShort
        <FieldOffset(15)> <MarshalAsAttribute(UnmanagedType.U1)> Public frameLength As Byte
        <FieldOffset(16)> <MarshalAsAttribute(UnmanagedType.U2)> Public frameCount As UShort
        <FieldOffset(18)> <MarshalAsAttribute(UnmanagedType.U2)> Public interFrameDelay As UShort
    End Structure

#End Region

    '   Structure Variable
    Private txTest As _sPlcSimTxTestParams  'local
    Public Const HYBRII_CELLBUF_SIZE As Integer = 128

    '   <summary>
    '       Assigning different parameter values to the structure
    '   </summary>
#Region "Assign parameter values"
    '
    '   Assign frame type
    '
    Private Sub cmbbxFrmType_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbxFrmType.SelectedIndexChanged
        txTest.frmType = CType([Enum].Parse(GetType(eFrmType), cmbbxFrmType.Text), eFrmType)
    End Sub
    '
    '   Assign security test mode   
    '
    Private Sub cmbbxSecTestMode_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbxSecTestMode.SelectedIndexChanged

        If eSecTestMode.UNENCRYPTED = CType([Enum].Parse(GetType(eSecTestMode), cmbbxSecTestMode.Text), eSecTestMode) Then
            cmbbx_Eks.Items.Clear()
            For i As Integer = 0 To 7
                cmbbx_Eks.Items.Add(i)
            Next
            cmbbx_Eks.Items.Add("F")
            cmbbx_Eks.Enabled = False
            cmbbx_Eks.SelectedIndex = cmbbx_Eks.Items.Count - 1
            txTest.secTestMode = 15
        Else
            cmbbx_Eks.Enabled = True
            txTest.secTestMode = CType([Enum].Parse(GetType(eSecTestMode), cmbbxSecTestMode.Text), eSecTestMode)
            cmbbx_Eks.Items.Clear()
            For i As Integer = 0 To 7
                cmbbx_Eks.Items.Add(i)
            Next
            cmbbx_Eks.SelectedIndex = 0
        End If

    End Sub
    '
    '   Assign length test mode
    '
    Private Sub cmbbxLenTestMode_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbxLenTestMode.SelectedIndexChanged
        txTest.lenTestMode = CType([Enum].Parse(GetType(eLenTestMode), cmbbxLenTestMode.Text), eLenTestMode)
    End Sub
    '
    '   Assign continuous mode
    '    
    Private Sub chkbx_ContMode_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_ContMode.CheckedChanged
        If chkbx_ContMode.Checked Then
            txTest.contMode = &H1
        Else
            txTest.contMode = &H0
        End If
    End Sub
    '
    '   Assign Broadcast mode
    '
    Private Sub chkbx_BCastMode_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_BCastMode.CheckedChanged
        If chkbx_BCastMode.Checked Then
            txTest.mcstMode = &H1
        Else
            txTest.mcstMode = &H0
        End If
    End Sub
    ''
    ''   Assign Frame length
    ''
    'Private Sub txtbx_FrmLen_TextChanged(sender As Object, e As EventArgs) Handles txtbx_FrmLen.TextChanged
    '    txTest.frmLen = txtbx_FrmLen.Text
    'End Sub
    ''
    ''   Assign inter frame delay
    ''
    'Private Sub txtbx_InterFrmDelay_TextChanged(sender As Object, e As EventArgs) Handles txtbx_InterFrmDelay.TextChanged
    '    txTest.delay = txtbx_InterFrmDelay.Text
    'End Sub
    ''
    ''   Assign number of frames
    ''
    'Private Sub txtbx_NumOfFrms_TextChanged(sender As Object, e As EventArgs) Handles txtbx_NumOfFrms.TextChanged
    '    txTest.numFrames = txtbx_NumOfFrms.Text
    'End Sub
    '
    '   Assign key index
    '
    Private Sub cmbbx_Eks_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbx_Eks.SelectedIndexChanged
        If "F" = cmbbx_Eks.Text Then
            txTest.eks = 15
        Else
            txTest.eks = cmbbx_Eks.Text
        End If
    End Sub

#End Region

    '
    '   Save all the parametes
    '
    Public Sub btn_Save_Click(sender As Object, e As EventArgs) Handles btn_Save.Click

        '   Assign text box values
        txTest.frmLen = txtbx_FrmLen.Text
        txTest.delay = txtbx_InterFrmDelay.Text
        txTest.numFrames = txtbx_NumOfFrms.Text


        '   Default and not open to user
        txTest.snid = 1
        txTest.dtei = 2
        txTest.stei = 3

        txTest.descLen = HYBRII_CELLBUF_SIZE

        If HomeScreen.testParamsFor = HomeScreen.tests.TEST_ID_PLC_TX Then
            HomeScreen.plcTXparams = txTest
            HomeScreen.userPLCTXPramasSet = True
        ElseIf HomeScreen.testParamsFor = HomeScreen.tests.TEST_ID_PLC_RX Then
            HomeScreen.plcRXparams = txTest
            HomeScreen.userPLCRXPramasSet = True
        End If

        'HomeScreen.gtxTest = txTest
        txTest = Nothing
        Me.Dispose()
    End Sub
    '
    '   Form dispose
    '
    Private Sub btn_Cancle_Click(sender As Object, e As EventArgs) Handles btn_Cancle.Click
        txTest = Nothing
        Threading.Thread.Sleep(10)
        Me.Dispose()
    End Sub
    '
    '   Main Form Init
    '
    Private Sub spiTXTestSettings_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        cmbbxFrmType.SelectedIndex = 0
        cmbbxLenTestMode.SelectedIndex = 0
        cmbbxSecTestMode.SelectedIndex = 0
        cmbbxFrmType.SelectedIndex = 0

        '   Disable cont mode
        cmbbx_Eks.Enabled = False
        chkbx_ContMode.Enabled = False

        If HomeScreen.chkbx_txSweep.Checked = True Then

            txtbx_FrmLen.AppendText("100")
            txtbx_InterFrmDelay.AppendText("4")
            txtbx_NumOfFrms.AppendText("1000")

            chkbx_BCastMode.Enabled = False

            cmbbx_Eks.SelectedIndex = 0
            cmbbxLenTestMode.SelectedIndex = 3
            cmbbxSecTestMode.SelectedIndex = 1

            cmbbxFrmType.Enabled = False
            cmbbxLenTestMode.Enabled = False
            cmbbxSecTestMode.Enabled = False
            cmbbxFrmType.Enabled = False

            txtbx_FrmLen.Enabled = False
            txtbx_InterFrmDelay.Enabled = False
            txtbx_NumOfFrms.Enabled = False

            btn_Save.Enabled = False
            btn_Cancle.Text = "OK"

        ElseIf HomeScreen.chkbx_rxSweep.Checked = True Then

            txtbx_FrmLen.AppendText("100")
            txtbx_InterFrmDelay.AppendText("4")
            txtbx_NumOfFrms.AppendText("1000")

            chkbx_BCastMode.Enabled = False

            cmbbx_Eks.SelectedIndex = 0
            cmbbxLenTestMode.SelectedIndex = 3
            cmbbxSecTestMode.SelectedIndex = 1

            cmbbxFrmType.Enabled = False
            cmbbxLenTestMode.Enabled = False
            cmbbxSecTestMode.Enabled = False
            cmbbxFrmType.Enabled = False

            txtbx_FrmLen.Enabled = False
            txtbx_InterFrmDelay.Enabled = False
            txtbx_NumOfFrms.Enabled = False

            btn_Save.Enabled = False
            btn_Cancle.Text = "OK"

        End If
    End Sub

End Class