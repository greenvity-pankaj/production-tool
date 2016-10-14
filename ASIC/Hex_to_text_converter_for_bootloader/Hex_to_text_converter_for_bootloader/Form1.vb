Imports System.IO


Public Class Form1
    Public openfile As String
    Public selectfile_path As String
    Public hex_file As String
    Public bin_file As String
    Public fs As FileStream
    Public sr As StreamReader
    Public device_mem As Integer
    Public hex_array(&H1FFF) As Byte
    'Public size As Integer
    Public data1 As String
    Public ascii_data As String
    Dim Style As MsgBoxStyle
    Public Const VIDPID = "vid_04d8&pid_000c"
    

    
    
    Private Sub OpenToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles OpenToolStripMenuItem.Click
        

        Dim ext As String
        Dim dot_pos As Integer

        If OpenFileDialog1.ShowDialog = DialogResult.OK Then
            openfile = OpenFileDialog1.FileName

            selectfile_path = Path.GetDirectoryName(openfile)
            lbl_file.Text = "File:" & openfile


            dot_pos = InStr(1, StrReverse(openfile), ".", vbBinaryCompare)
            ext = UCase(Strings.Right(openfile, (dot_pos - 1)))
            If (ext = "HEX") Then
                Call open_hexfile(openfile)
            ElseIf (ext = "BIN") Then
                Call open_binfile(openfile)
            End If

        End If


    End Sub

    Private Sub SaveToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles SaveToolStripMenuItem.Click
        SaveFileDialog1.ShowDialog()

    End Sub

    Private Sub btn_open_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_open.Click
        Dim ext As String
        Dim dot_pos As Integer

        If OpenFileDialog1.ShowDialog = DialogResult.OK Then
            openfile = OpenFileDialog1.FileName

            selectfile_path = Path.GetDirectoryName(openfile)
            lbl_file.Text = "File:" & openfile


            dot_pos = InStr(1, StrReverse(openfile), ".", vbBinaryCompare)
            ext = UCase(Strings.Right(openfile, (dot_pos - 1)))
            If (ext = "HEX") Then
                Call open_hexfile(openfile)
            ElseIf (ext = "BIN") Then
                Call open_binfile(openfile)
            End If

        End If

    End Sub

    Private Sub btn_hexview_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles btn_hexview.Click
        Form2.Show()

    End Sub

    Sub hex_editor_sub()
        Dim i As Integer
        Dim chksum, eof As Integer

        Dim no_of_bytes, j As Integer
        Dim address As Integer
        Dim Temp_char As String

        For i = 0 To (hex_file.Length - 1)


            If (hex_file.Chars(i) = ":") Then

                Temp_char = Mid(hex_file, i + 2, 2)
                no_of_bytes = CLng("&H" & Temp_char)

                Temp_char = Mid(hex_file, i + 4, 4)
                address = CLng("&H" & Temp_char)

                Temp_char = Mid(hex_file, i + 8, 2)
                eof = CLng("&H" & Temp_char)

               
                For j = 0 To (no_of_bytes - 1)

                    Temp_char = Mid(hex_file, i + 10 + j * 2, 2)

                    hex_array(address) = CByte(HEXtoBinary(Temp_char))

                    address = address + 1

                Next

                Temp_char = Mid(hex_file, i + 10 + j * 2, 2)
                chksum = CByte("&H" & Temp_char)

            End If
        Next

    End Sub

    Private Sub open_hexfile(ByVal openfile As String)
        Dim i As Integer
        Dim j As Integer
        Dim k As Integer
        Dim address As Integer
        Dim address_string As String
        Dim data As String
        Dim data2(device_mem / &H10) As String


        fs = New FileStream(openfile, FileMode.Open)
        sr = New StreamReader(fs)
        hex_file = sr.ReadToEnd()
        fs.Close()
        sr.Close()
        For i = 0 To &H1FFF
            If chkbx_buff_init.Checked = False Then
                hex_array(i) = &HFF      ' clear buffer
            Else
                hex_array(i) = &H0
            End If
        Next
        'TextBox1.Text = sr.ReadToEnd()

        hex_editor_sub()
        j = 0
        TextBox1.Text = " File Open Ok."
        Data = ""
        data1 = ""
        address_string = ""
        ascii_data = ""
        address = 0
        k = 0

        For i = 0 To device_mem

            If hex_array(i) > &HF Then

                Data = Data & vbTab & (hex_array(i).ToString("X"))
            Else

                Data = Data & vbTab & "0" & (hex_array(i).ToString("X"))
            End If

            If ((hex_array(i) <= 31) Or (hex_array(i) >= &H7F)) Then
                ascii_data = ascii_data + "  " + "."
            Else
                ascii_data = ascii_data + "  " + UCase(Chr(hex_array(i)))

            End If

            j = j + 1
            If j = 16 Then
                If address <= &HF Then
                    address_string = "000" & (address.ToString("X"))
                ElseIf address <= &HFF Then
                    address_string = "00" & (address.ToString("X"))
                ElseIf address <= &HFFF Then
                    address_string = "0" & (address.ToString("X"))
                Else
                    address_string = address.ToString("X")
                End If

                k = k + 1
                If k <= (device_mem / &H10) Then
                    data2(k) = address_string & vbTab & data & vbTab & ascii_data
                    data1 = data1 + data2(k) + vbCrLf
                    Data = ""
                    ascii_data = ""
                Else
                    k = 0
                End If

                j = 0
                address = address + &H10

            End If

        Next
    End Sub
    Private Sub open_binfile(ByVal openfile As String)
        ' fs = New FileStream(openfile, FileMode.Open)
        'sr = New StreamReader(fs)
        'bin_file = sr.ReadToEnd()
        'fs.Close()
        'sr.Close()

        Dim i As Integer
        Dim j As Integer
        Dim k As Integer
        Dim address As Integer
        Dim address_string As String
        Dim data As String
        Dim data2(device_mem / &H10) As String


        Dim binaryin As New BinaryReader(New FileStream(openfile, FileMode.OpenOrCreate, FileAccess.Read))


        For i = 0 To device_mem

            hex_array(i) = &HFF      ' clear buffer'
        Next

        'hex_array(i) = CByte(Mid(bin_file.Chars(i), i, 2))
        '  If binaryin.PeekChar <> -1 Then
        'hex_array(i) = binaryin.ReadByte
        'Debug.Print(hex_array(i))
        'Else
        '      Exit For
        ' End If
        Do While binaryin.PeekChar <> -1
            bin_file = bin_file & binaryin.ReadString

        Loop


        binaryin.Close()
        Debug.Print(bin_file)
        For i = 0 To &H1FFF

            hex_array(i) = &HFF      ' clear buffer'
            hex_array(i) = CByte(Mid(bin_file.Chars(i), i, 2))
        Next

        'hex_array(i) = CByte(Mid(bin_file.Chars(i), i, 2))
        '''''''''''''''''''''''
        j = 0
        TextBox1.Text = " File Open Ok."
        data = ""
        data1 = ""
        address_string = ""
        ascii_data = ""
        address = 0
        k = 0

        For i = 0 To device_mem

            If hex_array(i) > &HF Then

                data = data & vbTab & (hex_array(i).ToString("X"))
            Else

                data = data & vbTab & "0" & (hex_array(i).ToString("X"))
            End If

            If ((hex_array(i) <= 31) Or (hex_array(i) >= &H7F)) Then
                ascii_data = ascii_data + "  " + "."
            Else
                ascii_data = ascii_data + "  " + UCase(Chr(hex_array(i)))

            End If

            j = j + 1
            If j = 16 Then
                If address <= &HF Then
                    address_string = "000" & (address.ToString("X"))
                ElseIf address <= &HFF Then
                    address_string = "00" & (address.ToString("X"))
                ElseIf address <= &HFFF Then
                    address_string = "0" & (address.ToString("X"))
                Else
                    address_string = address.ToString("X")
                End If

                k = k + 1
                If k <= (device_mem / &H10) Then
                    data2(k) = address_string & vbTab & data & vbTab & ascii_data
                    data1 = data1 + data2(k) + vbCrLf
                    data = ""
                    ascii_data = ""
                Else
                    k = 0
                End If

                j = 0
                address = address + &H10

            End If

        Next
        ''''''''''''''''''''''''''''''

    End Sub

    Public Function HEXtoBinary(ByVal hex_value As String) As Long
        HEXtoBinary = CLng("&H" & hex_value)
    End Function

    Private Sub Form1_Load(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Load

        device_mem = &H1FFF     ' this field has to be modified from data base
    End Sub

    Private Sub AboutToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles AboutToolStripMenuItem.Click
        AboutBox1.ShowDialog()

    End Sub

    Private Sub btn_program_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)

        Dim result As MsgBoxResult

        Style = MsgBoxStyle.YesNo Or MsgBoxStyle.Question Or MsgBoxStyle.DefaultButton2

        result = MsgBox("Do You Want to ERASE Device", Style)
        If result = MsgBoxResult.No Then
            MsgBox("Programming Aborted by User", MsgBoxStyle.Critical)
            Exit Sub
        Else
            MsgBox("Erasing Device", MsgBoxStyle.Information)
        End If
    End Sub

    Private Sub btn_save_text_Click(sender As Object, e As EventArgs) Handles btn_save_text.Click
        Dim SaveFile As String
        Dim hex_data As String = ""
        If SaveFileDialog2.ShowDialog() = DialogResult.OK Then
            SaveFile = SaveFileDialog2.FileName
            ' MsgBox(SaveFile)
            ' hex_data = (hex_array(0).ToString("x"))



            If hex_array(0) > &HF Then
                If chkbx_text_case.Checked = False Then
                    hex_data = hex_data & (hex_array(0).ToString("x"))
                Else
                    hex_data = hex_data & (hex_array(0).ToString("X"))
                End If

            Else
                If chkbx_text_case.Checked = False Then
                    hex_data = hex_data & "0" & (hex_array(0).ToString("x"))
                Else
                    hex_data = hex_data & "0" & (hex_array(0).ToString("X"))
                End If
            End If

                For i = 1 To device_mem
                'hex_data = hex_data & vbCrLf & (hex_array(i).ToString("x"))

                If chkbx_8x1.Checked = True Then

                    If hex_array(i) > &HF Then
                        ' If (i And 8) Then
                        If chkbx_text_case.Checked = False Then
                            hex_data = hex_data & vbCrLf & (hex_array(i).ToString("x"))
                        Else
                            hex_data = hex_data & vbCrLf & (hex_array(i).ToString("X"))
                        End If

                    Else
                        ' If (i And 8) Then
                        If chkbx_text_case.Checked = False Then
                            hex_data = hex_data & vbCrLf & "0" & (hex_array(i).ToString("x"))
                        Else
                            hex_data = hex_data & vbCrLf & "0" & (hex_array(i).ToString("X"))
                        End If
                    End If

                Else
                    If hex_array(i) > &HF Then
                        If (i Mod 8) = 0 Then
                            If chkbx_text_case.Checked = False Then
                                hex_data = hex_data & " " & vbCrLf & (hex_array(i).ToString("x"))
                            Else
                                hex_data = hex_data & " " & vbCrLf & (hex_array(i).ToString("X"))
                            End If

                        Else
                            If chkbx_text_case.Checked = False Then
                                hex_data = hex_data & " " & (hex_array(i).ToString("x"))
                            Else
                                hex_data = hex_data & " " & (hex_array(i).ToString("X"))
                            End If

                        End If

                    Else
                        If (i Mod 8) = 0 Then
                            If chkbx_text_case.Checked = False Then
                                hex_data = hex_data & " " & vbCrLf & "0" & (hex_array(i).ToString("x"))
                            Else
                                hex_data = hex_data & " " & vbCrLf & "0" & (hex_array(i).ToString("X"))
                            End If

                        Else
                            If chkbx_text_case.Checked = False Then
                                hex_data = hex_data & " " & "0" & (hex_array(i).ToString("x"))
                            Else
                                hex_data = hex_data & " " & "0" & (hex_array(i).ToString("X"))
                            End If

                        End If

                    End If
                End If
            Next

            Try
                My.Computer.FileSystem.WriteAllText(
            SaveFileDialog2.FileName, hex_data, True, System.Text.Encoding.ASCII)
            Catch ex As Exception
                TextBox1.Text = TextBox1.Text & vbCrLf & ex.ToString

            End Try

            TextBox1.Text = TextBox1.Text & vbCrLf & "File Translation done!"
        End If
    End Sub
End Class
'FileStream.Seek(offset, origin) offset= begin,current,end