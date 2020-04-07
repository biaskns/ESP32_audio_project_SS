

%% Reading audiofile into int16

[data, fs] = audioread('LeftRight.wav', 'native');

%% Power of original
TotalEnergy = 0
for n = 1:length(data)
    Energy = double((data(n,1)+data(n,2)))^2;
    TotalEnergy = TotalEnergy + Energy;
end

Power_original = TotalEnergy/length(data)


%% Listen to original sound
[toplay, fs2] = audioread('LeftRight.wav');
sound(toplay, fs2)

%% Write read audiofile to format

% Convert to single coloumn

serialized = zeros(1, 2*length(data), 'int16');
row = 1;
for n = 1:2*length(data)
    if(mod(n, 2) == 1)
        serialized(n) = data(row,1);
    else
        serialized(n) = data(row,2);
        row = row + 1;
    end
end
serialized = serialized';

fid = fopen('InputFile.data', 'w');
if fid == -1
    error('Could not open input file')
end

fwrite(fid, serialized, 'int16');
fclose(fid);

%% Read data from DSP program

fid = fopen('OutputFile.data');
if fid == -1
    error('Could not open output file')
end

data_new = fread(fid, length(serialized), 'int16=>int16', 'ieee-be');

fclose(fid);

%% Deserialize

% Convert into two channels

deserialized = zeros(length(data), 2, 'int16');

row = 1;
for n = 1:length(serialized)
    if(mod(n, 2) == 1)
        deserialized(row, 1) = data_new(n);
    else
        deserialized(row, 2) = data_new(n);
        row = row + 1;
    end
end

%% Power of processed data

TotalEnergy = 0
for n = 1:length(data)
    Energy = double((deserialized(n,1)+deserialized(n,2)))^2;
    TotalEnergy = TotalEnergy + Energy;
end

Power_processed = TotalEnergy/length(data)

%% Audio analysis

left_channel = deserialized(:,1);
right_channel = deserialized(:,2);
N_samples_left = length(left_channel);
N_samples_right = length(right_channel);
fft_left = abs(fft(left_channel));

freq_axis = (0:(N_samples_left/2)-1)*(fs/N_samples_left);

figure(1); clf
semilogx(freq_axis, 20*log(fft_left(1:N_samples_left/2)))
hold on
grid on
xlim([1 24000])
hold off

%% Convert Read data to audio file

audiowrite('outputsound.wav', deserialized, 48000);

%% Is equal
isequal(data, deserialized)