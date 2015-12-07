Imports System, System.Text, System.Net, System.Net.Sockets

Public Class ConnectedClient

    Const BYTES_TO_READ As Integer = 512
    Public mClient As System.Net.Sockets.TcpClient
    Public mDevType As UInteger
    Private readThread As System.Threading.Thread
    Private Const MESSAGE_DELIMITER As Char = ControlChars.Cr

    Private lockObj = New Object()

    Public Event dataReceived(ByVal sender As ConnectedClient, ByVal message As Byte())
    '
    '   Sub
    '
    Sub New(ByVal client As System.Net.Sockets.TcpClient)

        mClient = client

        readThread = New System.Threading.Thread(AddressOf doRead)
        readThread.IsBackground = True
        readThread.Start()
    End Sub
    '
    '   Clear connection and terminate thread
    '
    Private Sub clearConnection()
        mClient.GetStream.Flush()
        mClient.GetStream.Close()
        mClient.Close()
        readThread.Abort()
    End Sub
    '
    '   Read data
    '
    Private Sub doRead()

        Dim readBuffer As Byte() = New Byte(BYTES_TO_READ) {}
        Dim bytesRead = New Integer

        Do
            Try
                If mClient.Connected Then
                    If mClient.GetStream.CanRead Then
                        bytesRead = mClient.GetStream.Read(readBuffer, 0, BYTES_TO_READ)
                        If (bytesRead > 0) Then

                            SyncLock lockObj
                                RaiseEvent dataReceived(Me, readBuffer)
                            End SyncLock

                            mClient.GetStream.Flush()
                        End If

                        If bytesRead = 0 Then
                            clearConnection()
                        End If

                        Array.Clear(readBuffer, 0, BYTES_TO_READ)
                        bytesRead = 0

                    Else    ' if networkstream is not readable
                        clearConnection()
                    End If
                End If

            Catch ex As SocketException

                If ex.NativeErrorCode.Equals(10053) Then
                    clearConnection()
                    Array.Clear(readBuffer, 0, BYTES_TO_READ)
                    bytesRead = 0
                End If

            End Try
            Threading.Thread.Sleep(5)
        Loop

    End Sub

    Public Sub SendMessage(ByVal msg As Byte())

        Try
            SyncLock lockObj
                If mClient.GetStream.CanWrite Then
                    mClient.GetStream.Write(msg, 0, msg.Length)
                End If
            End SyncLock
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub
End Class
