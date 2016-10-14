Option Strict Off
Option Explicit On
Imports System
Imports System.Runtime.InteropServices

Module Module1
    ' IMPORTANT note while using newer version of Mpusbapi.dll provided by Microchip USB Stack
    ' It is necessory to provide EntryPoint to dll other wise it gives runtime exception error

    ' Declare Function MPUSBOpen Lib "C:\windows\System32\Mpusbapi.dll" (ByVal inst As Integer, ByVal VIDPID As String, ByVal Pep As String, ByVal dwdir As Integer, ByVal dwres As Integer) As Integer

    'Declare Function MPUSBWrite Lib "C:\windows\System32\Mpusbapi.dll" (ByVal hand As Integer, ByRef datsend As Byte, ByVal tosd As Integer, ByRef sded As Integer, ByVal tout As Integer) As Integer

    'Declare Function MPUSBRead Lib "C:\windows\System32\Mpusbapi.dll" (ByVal hand As Integer, ByRef datrec As Byte, ByVal torec As Integer, ByRef recved As Integer, ByVal tout As Integer) As Integer

    ' Declare Function MPUSBGetDLLVersion Lib "C:\windows\System32\Mpusbapi.dll" () As Integer

    ' Declare Function MPUSBClose Lib "C:\windows\System32\Mpusbapi.dll" (ByVal instance As Integer) As Boolean

    'Declare Function MPUSBGetDeviceCount Lib "C:\windows\System32\Mpusbapi_vb.dll" (ByVal VIDPID As String) As Short
    
    <DllImport("C:\windows\System32\Mpusbapi.dll", EntryPoint:="_MPUSBOpen")> _
    Function MPUSBOPEN(ByVal inst As Integer, ByVal VIDPID As String, ByVal Pep As String, ByVal dwdir As Integer, ByVal dwres As Integer) As Integer

    End Function

    <DllImport("C:\windows\System32\Mpusbapi.dll", EntryPoint:="_MPUSBWrite")> _
    Function MPUSBWrite(ByVal hand As Integer, ByRef datsend As Byte, ByVal tosd As Integer, ByRef sded As Integer, ByVal tout As Integer) As Integer
    End Function

    <DllImport("C:\windows\System32\Mpusbapi.dll", EntryPoint:="_MPUSBRead")> _
    Function MPUSBRead(ByVal hand As Integer, ByRef datrec As Byte, ByVal torec As Integer, ByRef recved As Integer, ByVal tout As Integer) As Integer
    End Function

    <DllImport("C:\windows\System32\Mpusbapi.dll", EntryPoint:="_MPUSBGetDLLVersion")> _
    Function MPUSBGetDLLVersion() As Integer
    End Function

    <DllImport("C:\windows\System32\Mpusbapi.dll", EntryPoint:="_MPUSBClose")> _
    Function MPUSBClose(ByVal instance As Integer) As Boolean
    End Function

    <DllImport("C:\windows\System32\Mpusbapi.dll", EntryPoint:="_MPUSBGetDeviceCount")> _
    Function MPUSBGetDeviceCount(ByVal VIDPID As String) As Short
    End Function

    Public Declare Function GetLastError Lib "kernel32.dll" () As Long

    Public Const INVALID_HANDLE_VALUE = -1
    Public Const ERROR_INVALID_HANDLE = 6&

    Public Const out_pipe_end_pt0 = "\MCHP_EP1" 'We don't want two \\ in VB... \\ is for C
    Public Const in_pipe_end_pt0 = "\MCHP_EP1"



    Public Const MPUSB_FAIL = 0
    Public Const MPUSB_SUCCESS = 1

    Public Const MP_WRITE = 0
    Public Const MP_READ = 1

    'Declare the IN PIPE and OUT PIPE Public variables
    Public myInPipe As Object
    Public myOutPipe As Long

    Public status As Boolean
    Public comm_error As Boolean
    Const VIDPID = "vid_04d8&pid_000c"

    Sub OpenMPUSBDevice()
        Dim tempPipe As Long
        Dim count As Long
        Dim message As String

        tempPipe = INVALID_HANDLE_VALUE
        count = MPUSBGetDeviceCount(VIDPID)

        If count > 0 Then

            myOutPipe = MPUSBOPEN(0, VIDPID, out_pipe_end_pt0, MP_WRITE, 0)
            myInPipe = MPUSBOPEN(0, VIDPID, in_pipe_end_pt0, MP_READ, 0)
            Debug.Print(myInPipe, myOutPipe)

            If myOutPipe = INVALID_HANDLE_VALUE Or myInPipe = INVALID_HANDLE_VALUE Then
                
                message = "Failed to open data pipes."
                MsgBox(message)
                'MsgBox(myOutPipe.ToString + myInPipe.ToString + "Failed to open data pipes.")
                myOutPipe = myInPipe = INVALID_HANDLE_VALUE
                comm_error = True

            Else
                status = True    ' to check device is connected or not
                comm_error = False
            End If
       
        End If

    End Sub

    Sub CloseMPUSBDevice()
        If myOutPipe <> INVALID_HANDLE_VALUE Then
            MPUSBClose(myOutPipe)
            myOutPipe = INVALID_HANDLE_VALUE
        End If
        If myInPipe <> INVALID_HANDLE_VALUE Then
            MPUSBClose(myInPipe)
            myInPipe = INVALID_HANDLE_VALUE
        End If

        status = False
        comm_error = False
    End Sub

    

    Sub CheckInvalidHandle()
        If (GetLastError() = ERROR_INVALID_HANDLE) Then

            '// Most likely cause of the error is the board was disconnected.

            CloseMPUSBDevice()
        Else
            MsgBox("Error Code : " + GetLastError().ToString)
            CloseMPUSBDevice()
            comm_error = True

        End If
    End Sub

End Module