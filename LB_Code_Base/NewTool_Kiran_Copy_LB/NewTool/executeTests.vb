'Imports System, System.Net, System.Net.Sockets, System.Windows.Forms
'Imports System.Text, System.IO, System.Threading
'Imports System.Collections, System.Collections.Generic, System.ComponentModel, System.Management
'Imports System.Runtime.InteropServices

'Public Class executeTests

'    '   <summary>
'    '       Structure to maintain the status of the execution
'    '   </summary>
'#Region "Execution Status"
'    'Public Structure execStatus
'    '    Public currentTest As HomeScreen.tests
'    '    Public TOOL_CMD_PREPARE_DUT As Byte
'    '    Public TOOL_CMD_PREPARE_DUT_CNF As Byte
'    '    Public TOOL_CMD_PREPARE_REFERENCE As Byte
'    '    Public TOOL_CMD_PREPARE_REFERENCE_CNF As Byte
'    '    Public TOOL_CMD_START_TEST As Byte
'    '    Public TOOL_CMD_START_TEST_CNF As Byte
'    '    Public TOOL_CMD_STOP_TEST As Byte
'    '    Public TOOL_CMD_STOP_TEST_CNF As Byte
'    '    Public TOOL_CMD_DEVICE_RESET As Byte
'    '    Public TOOL_CMD_DEVICE_RESET_CNF As Byte
'    '    Public TOOL_CMD_GET_RESULT As Byte
'    '    Public TOOL_CMD_GET_RESULT_CNF As Byte
'    '    Public TOOL_CMD_DEVICE_SEARCH As Byte
'    'End Structure
'#End Region

'    'Public currStatus As execStatus

'    '
'    '   Load
'    '
'    Sub New()

'    End Sub

'    '   Decide execution status based on the number of tests selected
'    '   Apply checks and conditions based on the tests
'    '   Decide rules per test to make generic
'    '   use dictionary

'    Public Sub exceuteTests(ByVal cl As ConnectedClient, ByVal t As HomeScreen.tests)

'        'Dim qExecStatus = New Queue(Of HomeScreen.tests)
'        'Dim execState = New Dictionary(Of HomeScreen.tests, RunTest.states)

'        'Dim t As HomeScreen.tests
'        't = HomeScreen.qExecStatus.Peek

'        Select Case t

'            Case HomeScreen.tests.TEST_ID_PLC_TX

'                Select Case HomeScreen.execState(t)

'                    Case RunTest.states.STATE_PREPARE_DUT

'                        For Each lvi As ListViewItem In HomeScreen.getSelectedLVIItem

'                            Dim ip = lvi.ToString.Split("{"c).Last.Split(" "c).First

'                            If CType(HomeScreen.clDirectory(ip), ConnectedClient).mDevType = RunTest.ClientType.DUT Then
'                                RunTest.beginSend(RunTest.states.STATE_PREPARE_DUT, HomeScreen.clDirectory(ip), t)
'                            End If

'                        Next

'                        Exit Select

'                    Case RunTest.states.STATE_PREPARE_REFERENCE

'                        For Each lvi As ListViewItem In HomeScreen.getSelectedLVIItem

'                            Dim ip = lvi.ToString.Split("{"c).Last.Split(" "c).First

'                            If CType(HomeScreen.clDirectory(ip), ConnectedClient).mDevType = RunTest.ClientType.REF Then
'                                RunTest.beginSend(RunTest.states.STATE_PREPARE_REFERENCE, HomeScreen.clDirectory(ip), t)
'                            End If

'                        Next

'                        Exit Select

'                    Case RunTest.states.STATE_START_TEST_DUT

'                        For Each lvi As ListViewItem In HomeScreen.getSelectedLVIItem
'                            Dim ip = lvi.ToString.Split("{"c).Last.Split(" "c).First

'                            If CType(HomeScreen.clDirectory(ip), ConnectedClient).mDevType = RunTest.ClientType.DUT Then
'                                RunTest.beginSend(RunTest.states.STATE_START_TEST, HomeScreen.clDirectory(ip), t)
'                            End If
'                        Next

'                        Exit Select

'                    Case RunTest.states.STATE_START_TEST_REF

'                        For Each lvi As ListViewItem In HomeScreen.getSelectedLVIItem
'                            Dim ip = lvi.ToString.Split("{"c).Last.Split(" "c).First

'                            If CType(HomeScreen.clDirectory(ip), ConnectedClient).mDevType = RunTest.ClientType.REF Then
'                                RunTest.beginSend(RunTest.states.STATE_START_TEST, HomeScreen.clDirectory(ip), t)
'                            End If
'                        Next

'                        Exit Select

'                    Case RunTest.states.STATE_STOP_TEST

'                        For Each lvi As ListViewItem In HomeScreen.getSelectedLVIItem
'                            Dim ip = lvi.ToString.Split("{"c).Last.Split(" "c).First

'                            If CType(HomeScreen.clDirectory(ip), ConnectedClient).mDevType = RunTest.ClientType.REF Then
'                                RunTest.beginSend(RunTest.states.STATE_STOP_TEST, HomeScreen.clDirectory(ip), t)
'                            End If
'                        Next

'                        Exit Select

'                    Case RunTest.states.STATE_GET_RESULT

'                        RunTest.beginSend(RunTest.states.STATE_GET_RESULT, cl, t)

'                        Exit Select

'                    Case RunTest.states.STATE_GET_RESULT_CNF
'                        HomeScreen.qExecStatus.Dequeue()
'                        Exit Select

'                End Select

'                Exit Select
'                '
'                '   Case PLC TX ends
'                '
'            Case HomeScreen.tests.TEST_ID_PLC_RX


'                Exit Select
'                '    
'                '   Case PLC RX ends
'                '
'        End Select
'        '   End of one test select switch

'    End Sub

'End Class
